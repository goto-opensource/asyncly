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

#include "asyncly/executor/detail/ThreadPoolExecutor.h"

namespace asyncly {

std::shared_ptr<ThreadPoolExecutor>
ThreadPoolExecutor::create(const asyncly::ISchedulerPtr& scheduler)
{
    auto executor = std::shared_ptr<ThreadPoolExecutor>(new ThreadPoolExecutor(scheduler));
    return executor;
}

ThreadPoolExecutor::ThreadPoolExecutor(const asyncly::ISchedulerPtr& scheduler)
    : m_activeThreads(0)
    , m_isShutdownActive(false)
    , m_isStopped(false)
    , m_scheduler(scheduler)
{
}

clock_type::time_point ThreadPoolExecutor::now() const
{
    return m_scheduler->now();
}

std::shared_ptr<Cancelable>
ThreadPoolExecutor::post_at(const clock_type::time_point& absTime, Task&& task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }
    task.maybe_set_executor(shared_from_this());
    return m_scheduler->execute_at(shared_from_this(), absTime, std::move(task));
}

std::shared_ptr<Cancelable>
ThreadPoolExecutor::post_after(const clock_type::duration& relTime, Task&& task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }
    task.maybe_set_executor(shared_from_this());
    return m_scheduler->execute_after(shared_from_this(), relTime, std::move(task));
}

std::shared_ptr<Cancelable>
ThreadPoolExecutor::post_periodically(const clock_type::duration& period, CopyableTask task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }
    return detail::PeriodicTask::create(period, std::move(task), shared_from_this());
}

asyncly::ISchedulerPtr ThreadPoolExecutor::get_scheduler() const
{
    return m_scheduler;
}

void ThreadPoolExecutor::post(Task&& closure)
{
    if (!closure) {
        throw std::runtime_error("invalid closure");
    }
    closure.maybe_set_executor(shared_from_this());
    {
        std::lock_guard<boost::mutex> lock(m_mutex);
        if (m_isStopped) {
            throw std::runtime_error("executor stopped");
        }
        m_taskQueue.push(std::move(closure));
    }
    m_condition.notify_one();
}

void ThreadPoolExecutor::finish()
{
    {
        std::lock_guard<boost::mutex> lock(m_mutex);
        m_isShutdownActive = true;
    }
    m_condition.notify_all();
}

void ThreadPoolExecutor::run()
{
    {
        std::lock_guard<boost::mutex> lock{ m_mutex };
        if (m_isStopped) {
            return;
        }
        ++m_activeThreads;
    }
    while (true) {
        boost::unique_lock<boost::mutex> lock{ m_mutex };

        m_condition.wait(lock, [this] {
            return !m_taskQueue.empty() || (m_isShutdownActive && m_taskQueue.empty());
        }); // wait does not throw since C++14

        if (m_isShutdownActive && m_taskQueue.empty()) {
            assert(!m_isStopped);
            if (--m_activeThreads == 0) {
                m_isStopped = true; // last thread sets stopped on exit
            }
            break;
        }
        auto task = std::move(m_taskQueue.front());
        m_taskQueue.pop();
        lock.unlock();
        task();
    }
}

}