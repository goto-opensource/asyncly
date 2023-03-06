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

#include <boost/thread/condition_variable.hpp>

#include <mutex>
#include <optional>
#include <queue>
#include <thread>

#include "FakeClockScheduler.h"
#include "IFakeExecutor.h"
#include "asyncly/ExecutorTypes.h"
#include "asyncly/executor/IStrand.h"
#include "asyncly/scheduler/IScheduler.h"
#include "asyncly/task/detail/PeriodicTask.h"

namespace asyncly::test {

class FakeExecutor;
using FakeExecutorPtr = std::shared_ptr<FakeExecutor>;

/// FakeExecutor is useful for single-threaded unit tests where tasks
/// should be executed in the same thread like the test code itself.
/// Main difference to ThreadPoolExecutor is that advanceClock() and runOnce() execute all pending
/// tasks and return(!). Typically, you would prepare your test by creating objects and proxies, set
/// your expectations, call one or multiple proxy methods and finally start execution by
/// advanceClock()/runOnce()
class FakeExecutor : public IFakeExecutor, public std::enable_shared_from_this<FakeExecutor> {
  public:
    static FakeExecutorPtr create();

    FakeExecutor(FakeExecutor const&) = delete;
    FakeExecutor& operator=(FakeExecutor const&) = delete;

    // IFakeExecutor
    void advanceClock(clock_type::duration advance) override;
    void advanceClock(clock_type::time_point timePoint) override;
    void advanceClockToCurrentLastEvent() override;
    size_t runTasks(size_t maxTasksToExecute = 0) override;
    void clear() override;
    size_t queuedTasks() const override;

    // IStrand
    clock_type::time_point now() const override;
    void post(Task&&) override;
    std::shared_ptr<Cancelable> post_at(const clock_type::time_point& absTime, Task&&) override;
    std::shared_ptr<Cancelable> post_after(const clock_type::duration& relTime, Task&&) override;
    [[nodiscard]] std::shared_ptr<AutoCancelable>
    post_periodically(const clock_type::duration& period, RepeatableTask&&) override;
    ISchedulerPtr get_scheduler() const override;

  private:
    FakeExecutor();

    std::optional<std::thread::id> m_runningThreadId;
    std::queue<Task> m_taskQueue;
    std::shared_ptr<FakeClockScheduler> m_scheduler;
    bool m_taskRunning;
};

inline FakeExecutorPtr FakeExecutor::create()
{
    return std::shared_ptr<FakeExecutor>(new FakeExecutor());
}

inline FakeExecutor::FakeExecutor()
    : m_scheduler(std::make_shared<FakeClockScheduler>())
    , m_taskRunning(false)
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
    task.maybe_set_executor(weak_from_this());
    return m_scheduler->execute_at(weak_from_this(), absTime, std::move(task));
}

inline std::shared_ptr<Cancelable>
FakeExecutor::post_after(const clock_type::duration& relTime, Task&& task)
{
    if (!task) {
        throw std::runtime_error("invalid task");
    }
    task.maybe_set_executor(weak_from_this());
    return m_scheduler->execute_after(weak_from_this(), relTime, std::move(task));
}

inline std::shared_ptr<AutoCancelable>
FakeExecutor::post_periodically(const clock_type::duration& period, RepeatableTask&& task)
{
    if (!task) {
        throw std::runtime_error("invalid task");
    }
    return std::make_shared<AutoCancelable>(
        detail::PeriodicTask::create(period, std::move(task), shared_from_this()));
}

inline asyncly::ISchedulerPtr FakeExecutor::get_scheduler() const
{
    return m_scheduler;
}

inline void FakeExecutor::post(Task&& closure)
{
    if (!closure) {
        throw std::runtime_error("invalid task");
    }
    closure.maybe_set_executor(weak_from_this());
    m_taskQueue.push(std::move(closure));
}

inline void FakeExecutor::advanceClock(clock_type::duration advance)
{
    const auto timePoint = now() + advance;

    advanceClock(timePoint);
}

inline void FakeExecutor::advanceClock(clock_type::time_point advanceTimePoint)
{
    if (m_taskRunning) {
        m_scheduler->setClock(advanceTimePoint);
        return;
    }

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
    if (!m_runningThreadId.has_value()) {
        m_runningThreadId = currentThreadId;
    }
    if (m_runningThreadId != currentThreadId) {
        throw std::runtime_error("FakeExecutor can only be called from a single thread!");
    }

    m_taskRunning = true;
    auto size = m_taskQueue.size();
    auto remainingTasks = maxTasksToExecute;
    while (!m_taskQueue.empty() && (maxTasksToExecute == 0 || remainingTasks > 0)) {
        auto task = std::move(m_taskQueue.front());
        m_taskQueue.pop();
        task();
        --remainingTasks;
    }
    m_taskRunning = false;
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
} // namespace asyncly::test
