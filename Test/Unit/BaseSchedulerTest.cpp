/*
 * Copyright 2019 LogMeIn
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

#include "gmock/gmock.h"

#include "asyncly/scheduler/detail/BaseScheduler.h"

#include <algorithm>

namespace asyncly {

using namespace testing;

class ImmediatePostExecutor : public IExecutor {
  public:
    ImmediatePostExecutor(ISchedulerPtr scheduler)
        : _scheduler(std::move(scheduler))
    {
    }

    clock_type::time_point now() const override
    {
        throw;
    }
    void post(Task&& task) override
    {
        task();
    }
    std::shared_ptr<Cancelable> post_at(const clock_type::time_point&, Task&&) override
    {
        throw;
    }
    std::shared_ptr<Cancelable> post_after(const clock_type::duration&, Task&&) override
    {
        throw;
    }
    std::shared_ptr<Cancelable>
    post_periodically(const clock_type::duration&, CopyableTask) override
    {
        throw;
    }
    ISchedulerPtr get_scheduler() const override
    {
        return _scheduler;
    }

  private:
    ISchedulerPtr _scheduler;
};

class BaseSchedulerTest : public Test {
  public:
    BaseSchedulerTest()
        : _scheduler(std::make_shared<BaseScheduler>([this]() { return now; }))
        , _executor(std::make_shared<ImmediatePostExecutor>(_scheduler))
    {
    }

    void addTask(size_t index, clock_type::time_point timePoint)
    {
        _expectedTasks.insert(ExecutedTaskInfo{ index, timePoint });
        _scheduler->execute_at(_executor, timePoint, [this, index]() {
            _executedTasks.push_back(ExecutedTaskInfo{ index, now });
        });
    }
    void addTask(size_t index, clock_type::duration duration)
    {
        addTask(index, now + duration);
    }

    void advance(clock_type::duration duration)
    {
        auto maxDurationOffset = clock_type::duration::max() - now.time_since_epoch();
        duration = std::min(
            maxDurationOffset, duration); // limit duration to not exceed the type's max value

        auto advanceTimePoint = now + duration;
        do {
            auto nowBefore = now;
            now = _scheduler->getNextExpiredTime(advanceTimePoint);
            ASSERT_GE(now, nowBefore);
            _scheduler->prepareElapse();
            _scheduler->elapse();
        } while (now != advanceTimePoint);
    }

    void verify()
    {
        std::vector<ExecutedTaskInfo> expectedTasks;
        for (auto it = _expectedTasks.begin(); it != _expectedTasks.end();) {
            auto entry = *it;
            if (entry.timePoint <= now) {
                expectedTasks.push_back(entry);
                it = _expectedTasks.erase(it);
            } else {
                ++it;
            }
        }
        EXPECT_EQ(_executedTasks.size(), expectedTasks.size());
        for (auto it1 = _executedTasks.begin(), it2 = expectedTasks.begin();
             it1 != _executedTasks.end() && it2 != expectedTasks.end();
             ++it1, ++it2) {
            auto& actual = *it1;
            auto& expected = *it2;
            EXPECT_EQ(actual.index, expected.index);
            EXPECT_GE(actual.timePoint, expected.timePoint);
        }
    }

    void advanceAndVerify(clock_type::duration duration)
    {
        advance(duration);
        verify();
        _executedTasks.clear();
    }

    void clear()
    {
        _expectedTasks.clear();
        _executedTasks.clear();
    }

  protected:
    std::shared_ptr<BaseScheduler> _scheduler;
    clock_type::time_point now;

  private:
    std::shared_ptr<ImmediatePostExecutor> _executor;
    struct ExecutedTaskInfo {
        size_t index;
        clock_type::time_point timePoint;

        bool operator<(ExecutedTaskInfo const& o) const
        {
            return timePoint < o.timePoint;
        }
    };
    std::vector<ExecutedTaskInfo> _executedTasks;
    std::set<ExecutedTaskInfo> _expectedTasks;
};

TEST_F(BaseSchedulerTest, shouldExecuteScheduledTask)
{
    addTask(1, std::chrono::milliseconds(5));
    advance(std::chrono::milliseconds(5));
    verify();
}

TEST_F(BaseSchedulerTest, shouldExecuteMultipleScheduledTasks)
{
    addTask(1, std::chrono::milliseconds(5));
    addTask(2, std::chrono::milliseconds(10));
    addTask(3, std::chrono::milliseconds(20));
    advanceAndVerify(clock_type::duration::max());
}

TEST_F(BaseSchedulerTest, shouldExecuteUnorderedScheduledTasks)
{
    addTask(2, std::chrono::milliseconds(10));
    addTask(3, std::chrono::milliseconds(20));
    addTask(1, std::chrono::milliseconds(5));
    advanceAndVerify(clock_type::duration::max());
}

TEST_F(BaseSchedulerTest, shouldExecuteWhenInterleavedInsertAndElapse)
{
    addTask(1, std::chrono::milliseconds(5));
    addTask(2, std::chrono::milliseconds(10));
    advanceAndVerify(std::chrono::milliseconds(8));
    addTask(3, std::chrono::milliseconds(20));
    advanceAndVerify(clock_type::duration::max());
}

TEST_F(BaseSchedulerTest, shouldExecutePastTimepoints)
{
    advanceAndVerify(std::chrono::milliseconds(8));
    addTask(1, clock_type::time_point{} + std::chrono::milliseconds(5));
    advanceAndVerify(std::chrono::milliseconds(8));
}

TEST_F(BaseSchedulerTest, shouldExecutePastAndFutureTimepoints)
{
    advanceAndVerify(std::chrono::milliseconds(8));
    addTask(1, clock_type::time_point{} + std::chrono::milliseconds(5));
    addTask(2, clock_type::time_point{} + std::chrono::milliseconds(10));
    advanceAndVerify(std::chrono::milliseconds(8));
}

TEST_F(BaseSchedulerTest, shouldGetNextExpiredTime)
{
    addTask(1, std::chrono::milliseconds(5));
    addTask(2, std::chrono::milliseconds(10));
    const auto next = now + std::chrono::milliseconds(5);
    const auto limit = now + std::chrono::milliseconds(11);
    EXPECT_EQ(next, _scheduler->getNextExpiredTime(limit));
}

TEST_F(BaseSchedulerTest, shouldGetLimitWhenNoNextExpiredTimeBeforeLimit)
{
    addTask(1, std::chrono::milliseconds(5));
    addTask(2, std::chrono::milliseconds(10));
    const auto limit = now + std::chrono::milliseconds(3);
    EXPECT_EQ(limit, _scheduler->getNextExpiredTime(limit));
}

TEST_F(BaseSchedulerTest, shouldGetLastExpiredTime)
{
    addTask(1, std::chrono::milliseconds(5));
    addTask(2, std::chrono::milliseconds(10));
    addTask(3, std::chrono::milliseconds(20));
    addTask(4, std::chrono::milliseconds(15));
    addTask(5, std::chrono::milliseconds(17));
    const auto last = now + std::chrono::milliseconds(20);
    EXPECT_EQ(last, _scheduler->getLastExpiredTime());
}

TEST_F(BaseSchedulerTest, shouldGetNowWhenNoLastExpiredTime)
{
    EXPECT_EQ(now, _scheduler->getLastExpiredTime());
}

}
