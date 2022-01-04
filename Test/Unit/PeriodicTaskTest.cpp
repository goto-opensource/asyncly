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
const auto startTimePoint = std::chrono::steady_clock::time_point(0ms);
const auto period = 10ms;

} // namespace

struct DestructionSentinel {
  public:
    DestructionSentinel(std::promise<void>& destroyed)
        : _destroyed(destroyed)
    {
    }
    ~DestructionSentinel()
    {
        destroy();
        _destroyed.set_value();
    }
    MOCK_CONST_METHOD0(destroy, void());

    std::promise<void>& _destroyed;
};

TEST(
    PeriodicTaskTest,
    cancel_userSuppliedTaskIsExecutedConcurrently_userSuppliedTaskIsDeletedAfterExecutionHasFinished)
{
    auto fakeScheduler = std::make_shared<test::FakeClockScheduler>();
    auto controller = ThreadPoolExecutorController::create(2, fakeScheduler);
    auto executor = controller->get_executor();

    auto taskStarted = std::promise<void>();
    auto taskUnblocked = std::promise<void>();
    auto taskDestroyed = std::promise<void>();

    auto periodicTask = executor->post_periodically(
        period,
        [&taskStarted,
         &taskUnblocked,
         destructionSentinel = std::make_unique<DestructionSentinel>(taskDestroyed)]() mutable {
            EXPECT_CALL(*destructionSentinel, destroy).Times(0);
            taskStarted.set_value();
            taskUnblocked.get_future().get();
            EXPECT_CALL(*destructionSentinel, destroy).Times(1);
        });

    fakeScheduler->advanceClockToNextEvent(startTimePoint + period);

    // wait for the async task to start, cancel it and then unblock it
    taskStarted.get_future().get();
    periodicTask.reset();
    taskUnblocked.set_value();
    taskDestroyed.get_future().get();
}

TEST(
    PeriodicTaskTest,
    cancel_callFromWithinPeriodicTask_userSuppliedTaskIsDeletedAfterExecutionHasFinished)
{
    auto fakeScheduler = std::make_shared<test::FakeClockScheduler>();
    auto controller = ThreadPoolExecutorController::create(2, fakeScheduler);
    auto executor = controller->get_executor();

    auto taskDestroyed = std::promise<void>();

    std::shared_ptr<asyncly::AutoCancelable> periodicTask;
    periodicTask = executor->post_periodically(
        period,
        [&periodicTask,
         destructionSentinel = std::make_unique<DestructionSentinel>(taskDestroyed)]() mutable {
            EXPECT_CALL(*destructionSentinel, destroy).Times(0);
            periodicTask.reset();
            EXPECT_CALL(*destructionSentinel, destroy).Times(1);
        });

    fakeScheduler->advanceClockToNextEvent(startTimePoint + period);

    taskDestroyed.get_future().get();
}

} // namespace asyncly
