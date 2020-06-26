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

#pragma once

#include <boost/optional.hpp>
#include <boost/thread/condition_variable.hpp>

#include <mutex>
#include <queue>
#include <thread>

#include "FakeClockScheduler.h"
#include "asyncly/ExecutorTypes.h"
#include "asyncly/executor/IExecutor.h"
#include "asyncly/scheduler/IScheduler.h"
#include "asyncly/task/detail/PeriodicTask.h"

namespace asyncly {
namespace test {

class FakeExecutor;
using FakeExecutorPtr = std::shared_ptr<FakeExecutor>;

/// FakeExecutor is useful for single-threaded unit tests where tasks
/// should be executed in the same thread like the test code itself.
/// Main difference to ThreadPoolExecutor is that advanceClock() and runOnce() execute all pending
/// tasks and return(!). Typically, you would prepare your test by creating objects and proxies, set
/// your expectations, call one or multiple proxy methods and finally start execution by
/// advanceClock()/runOnce()
class FakeExecutor : public IExecutor, public std::enable_shared_from_this<FakeExecutor> {
  public:
    static FakeExecutorPtr create();

    FakeExecutor(FakeExecutor const&) = delete;
    FakeExecutor& operator=(FakeExecutor const&) = delete;

    void advanceClock(clock_type::duration advance);
    void advanceClock(clock_type::time_point timePoint);
    void advanceClockToCurrentLastEvent();
    /**
     * @param maxTasksToExecute default is 0 which means unlimited, > 0 means: execute only a
     * certain number of tasks in this call
     */
    size_t runTasks(size_t maxTasksToExecute = 0);
    void clear();
    size_t queuedTasks() const;

    clock_type::time_point now() const override;
    void post(Task&&) override;
    std::shared_ptr<Cancelable> post_at(const clock_type::time_point& absTime, Task&&) override;
    std::shared_ptr<Cancelable> post_after(const clock_type::duration& relTime, Task&&) override;
    std::shared_ptr<Cancelable>
    post_periodically(const clock_type::duration& period, CopyableTask) override;
    ISchedulerPtr get_scheduler() const override;
    bool is_serializing() const override;

  private:
    FakeExecutor();

    boost::optional<std::thread::id> m_runningThreadId;
    std::queue<Task> m_taskQueue;
    std::shared_ptr<FakeClockScheduler> m_scheduler;
};

inline FakeExecutorPtr FakeExecutor::create()
{
    return std::shared_ptr<FakeExecutor>(new FakeExecutor());
}

inline FakeExecutor::FakeExecutor()
    : m_scheduler(std::make_shared<FakeClockScheduler>())
{
}

inline clock_type::time_point FakeExecutor::now() const
{
    return m_scheduler->now();
}

inline std::shared_ptr<Cancelable>
FakeExecutor::post_at(const clock_type::time_point& absTime, Task&& task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }
    task.maybe_set_executor(shared_from_this());
    return m_scheduler->execute_at(shared_from_this(), absTime, std::move(task));
}

inline std::shared_ptr<Cancelable>
FakeExecutor::post_after(const clock_type::duration& relTime, Task&& task)
{
    if (!task) {
        throw std::runtime_error("invalid task");
    }
    task.maybe_set_executor(shared_from_this());
    return m_scheduler->execute_after(shared_from_this(), relTime, std::move(task));
}

inline std::shared_ptr<Cancelable>
FakeExecutor::post_periodically(const clock_type::duration& period, CopyableTask task)
{
    if (!task) {
        throw std::runtime_error("invalid task");
    }
    return detail::PeriodicTask::create(period, std::move(task), shared_from_this());
}

inline asyncly::ISchedulerPtr FakeExecutor::get_scheduler() const
{
    return m_scheduler;
}

inline bool FakeExecutor::is_serializing() const
{
    return true;
}

inline void FakeExecutor::post(Task&& closure)
{
    if (!closure) {
        throw std::runtime_error("invalid task");
    }
    closure.maybe_set_executor(shared_from_this());
    m_taskQueue.push(std::move(closure));
}

inline void FakeExecutor::advanceClock(clock_type::duration advance)
{
    const auto timePoint = now() + advance;

    advanceClock(timePoint);
}

inline void FakeExecutor::advanceClock(clock_type::time_point advanceTimePoint)
{
    runTasks();

    std::size_t executedTasks = 0u;
    bool done = false;
    do {
        done = m_scheduler->advanceClockToNextEvent(advanceTimePoint);
        executedTasks = runTasks();
    } while (executedTasks > 0 || !done);
}

inline void FakeExecutor::advanceClockToCurrentLastEvent()
{
    const auto last = m_scheduler->getLastExpiredTime();
    advanceClock(last);
}

inline size_t FakeExecutor::runTasks(size_t maxTasksToExecute /* = 0 */)
{
    const auto currentThreadId = std::this_thread::get_id();
    if (!m_runningThreadId.is_initialized()) {
        m_runningThreadId = currentThreadId;
    }
    if (m_runningThreadId != currentThreadId) {
        throw std::runtime_error("FakeExecutor can only be called from a single thread!");
    }

    auto size = m_taskQueue.size();
    auto remainingTasks = maxTasksToExecute;
    while (!m_taskQueue.empty() && (maxTasksToExecute == 0 || remainingTasks > 0)) {
        auto task = std::move(m_taskQueue.front());
        m_taskQueue.pop();
        task();
        --remainingTasks;
    }
    return size;
}

inline void FakeExecutor::clear()
{
    while (!m_taskQueue.empty()) {
        m_taskQueue.pop();
    }
    m_scheduler->clear();
}

inline size_t FakeExecutor::queuedTasks() const
{
    return m_taskQueue.size();
}
}
}
