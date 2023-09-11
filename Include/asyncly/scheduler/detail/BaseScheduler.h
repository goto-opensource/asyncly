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

#include <queue>

#include "asyncly/executor/ExecutorStoppedException.h"
#include "asyncly/executor/IExecutor.h"
#include "asyncly/scheduler/IScheduler.h"
#include "asyncly/scheduler/PriorityQueue.h"
#include "asyncly/task/CancelableTask.h"
#include "asyncly/task/Task.h"

namespace asyncly {

class BaseScheduler : public IScheduler {
  public:
    BaseScheduler(const ClockNowFunction& nowFunction);
    /**
     * prepareElapse() moves the elapsed scheduled tasks into a separate container which can be
     * executed by elapse()
     * prepareElapse() and elapse() should be called by the same thread (sequentially).
     */
    void prepareElapse();

    size_t elapse();

    size_t getQueueSize() const;
    /**
     * Return the time_point of the next scheduled tasks, if it is older or equal than limit.
     * Otherwise limit is returned.
     */
    clock_type::time_point getNextExpiredTime(clock_type::time_point limit) const;
    /**
     * Return the time_point of the last scheduled task but at least the current time (now).
     */
    clock_type::time_point getLastExpiredTime() const;

    void clear();

    // IScheduler
    clock_type::time_point now() const override;

    std::shared_ptr<Cancelable> execute_at(
        const IExecutorWPtr& executor, const clock_type::time_point& absTime, Task&&) override;

    std::shared_ptr<Cancelable> execute_after(
        const IExecutorWPtr& executor, const clock_type::duration& relTime, Task&&) override;

  private:
    struct TimerQueueElementCompare;
    using TimerQueueElement = std::pair<clock_type::time_point, Task>;
    using TimerQueue = detail::
        PriorityQueue<TimerQueueElement, std::vector<TimerQueueElement>, TimerQueueElementCompare>;

    struct TimerQueueElementCompare {
        bool operator()(const TimerQueueElement& a, const TimerQueueElement& b)
        {
            return a.first > b.first;
        }
    };

    TimerQueue m_timerQueue;
    std::queue<Task> m_elapsedQueue;
    ClockNowFunction m_now;
};

inline BaseScheduler::BaseScheduler(const ClockNowFunction& nowFunction)
    : m_now(nowFunction)
{
}

inline asyncly::clock_type::time_point BaseScheduler::now() const
{
    return m_now();
}

inline std::shared_ptr<Cancelable> BaseScheduler::execute_at(
    const IExecutorWPtr& executor, const clock_type::time_point& absTime, Task&& task)
{
    auto sharedTask = std::make_shared<Task>(std::move(task));
    auto cancelable = std::make_shared<TaskCancelable>(sharedTask);
    Task cancelableTask(CancelableTask(sharedTask, cancelable));

    m_timerQueue.push({ absTime, [executor, cancelableTask{ std::move(cancelableTask) }]() mutable {
                           if (auto p = executor.lock()) {
                               try {
                                   p->post(std::move(cancelableTask));
                               } catch (const ExecutorStoppedException&) {
                                   // ignore
                               }
                           }
                       } });
    return cancelable;
}

inline std::shared_ptr<Cancelable> BaseScheduler::execute_after(
    const IExecutorWPtr& executor, const clock_type::duration& relTime, Task&& closure)
{
    return execute_at(executor, m_now() + relTime, std::move(closure));
}

inline void BaseScheduler::prepareElapse()
{
    const auto now = m_now();
    while (!m_timerQueue.empty() && m_timerQueue.peek().first <= now) {
        m_elapsedQueue.push(std::move(m_timerQueue.pop().second));
    }
}

inline size_t BaseScheduler::elapse()
{
    const auto elapsedTasks = m_elapsedQueue.size();
    while (!m_elapsedQueue.empty()) {
        auto task = std::move(m_elapsedQueue.front());
        m_elapsedQueue.pop();
        task();
    }
    return elapsedTasks;
}
inline size_t BaseScheduler::getQueueSize() const
{
    return m_timerQueue.size();
}

inline clock_type::time_point BaseScheduler::getNextExpiredTime(clock_type::time_point limit) const
{
    const auto now = m_now();
    if (m_timerQueue.empty()) {
        return std::max(limit, now);
    } else if (m_timerQueue.peek().first <= limit) {
        return std::max(m_timerQueue.peek().first, now);
    } else {
        return std::max(limit, now);
    }
}

inline clock_type::time_point BaseScheduler::getLastExpiredTime() const
{
    const auto now = m_now();
    if (!m_timerQueue.empty()) {
        return std::max(m_timerQueue.peekBack().first, now);
    } else {
        return now;
    }
}

inline void BaseScheduler::clear()
{
    m_timerQueue.clear();
    while (!m_elapsedQueue.empty()) {
        m_elapsedQueue.pop();
    }
}
} // namespace asyncly
