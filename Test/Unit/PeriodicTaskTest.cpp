/*
 * Copyright 2021 LogMeIn
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <future>
#include <thread>
#include <unordered_map>

#include "gmock/gmock.h"

#include "asyncly/executor/ThreadPoolExecutorController.h"
#include "asyncly/test/FakeClockScheduler.h"

using namespace std::chrono_literals;

namespace asyncly {

namespace {
class Synchronizer {
  public:
    Synchronizer()
    {
        p.emplace_back(std::make_shared<std::promise<void>>());
    }

    void wait()
    {
        const auto id = std::this_thread::get_id();
        auto currentPromise = getCurrentPromise(id);
        currentPromise->get_future().get();
    }

    void notify()
    {
        const auto id = std::this_thread::get_id();
        auto currentPromise = getCurrentPromise(id);
        currentPromise->set_value();
    }

  private:
    std::shared_ptr<std::promise<void>> getCurrentPromise(std::thread::id id)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (indexPerThread.count(id) == 0) {
            indexPerThread[id] = 0;
        }
        if (indexPerThread[id] > p.size() - 1) {
            p.emplace_back(std::make_shared<std::promise<void>>());
        }
        return p[indexPerThread[id]++];
    }

    std::mutex mutex;
    std::vector<std::shared_ptr<std::promise<void>>> p;
    std::unordered_map<std::thread::id, size_t> indexPerThread;
};

const auto startTimePoint = std::chrono::steady_clock::time_point(0ms);
const auto period = 10ms;

}

struct DestructionSentinel {
  public:
    DestructionSentinel(Synchronizer& s)
        : m_synchronizer(s)
    {
    }
    ~DestructionSentinel()
    {
        destroy();
        m_synchronizer.notify();
    }
    MOCK_CONST_METHOD0(destroy, void());

    Synchronizer& m_synchronizer;
};

TEST(
    PeriodicTaskTest,
    cancel_userSuppliedTaskIsExecutedConcurrently_userSuppliedTaskIsDeletedAfterExecutionHasFinished)
{
    auto fakeScheduler = std::make_shared<test::FakeClockScheduler>();
    auto controller = ThreadPoolExecutorController::create(2, fakeScheduler);
    auto executor = controller->get_executor();

    auto synchronizer = Synchronizer{};

    auto periodicTask = executor->post_periodically(
        period,
        [&synchronizer,
         destructionSentinel = std::make_unique<DestructionSentinel>(synchronizer)]() mutable {
            EXPECT_CALL(*destructionSentinel, destroy).Times(0);
            synchronizer.notify();
            synchronizer.wait();
            EXPECT_CALL(*destructionSentinel, destroy).Times(1);
        });

    fakeScheduler->advanceClockToNextEvent(startTimePoint + period);

    // wait for the async task to start, cancel it and then unblock it
    synchronizer.wait();
    periodicTask->cancel();
    synchronizer.notify();
    synchronizer.wait();
}

TEST(
    PeriodicTaskTest,
    cancel_callFromWithinPeriodicTask_userSuppliedTaskIsDeletedAfterExecutionHasFinished)
{
    auto fakeScheduler = std::make_shared<test::FakeClockScheduler>();
    auto controller = ThreadPoolExecutorController::create(2, fakeScheduler);
    auto executor = controller->get_executor();

    auto synchronizer = Synchronizer{};

    std::shared_ptr<asyncly::Cancelable> periodicTask;
    periodicTask = executor->post_periodically(
        period,
        [&periodicTask,
         destructionSentinel = std::make_unique<DestructionSentinel>(synchronizer)]() mutable {
            EXPECT_CALL(*destructionSentinel, destroy).Times(0);
            periodicTask->cancel();
            EXPECT_CALL(*destructionSentinel, destroy).Times(1);
        });

    fakeScheduler->advanceClockToNextEvent(startTimePoint + period);

    synchronizer.wait();
}

}
