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

#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "asyncly/ExecutorTypes.h"
#include "asyncly/executor/ExecutorStoppedException.h"
#include "asyncly/executor/IExecutor.h"
#include "asyncly/scheduler/IScheduler.h"
#include "asyncly/task/detail/PeriodicTask.h"

namespace asyncly {

class IThreadPoolExecutor {
  public:
    virtual ~IThreadPoolExecutor() = default;
    virtual void run() = 0;
    virtual void finish() = 0;
};

template <typename Base>
class ThreadPoolExecutor final : public Base,
                                 public IThreadPoolExecutor,
                                 public std::enable_shared_from_this<ThreadPoolExecutor<Base>> {
  public:
    static std::shared_ptr<ThreadPoolExecutor> create(const asyncly::ISchedulerPtr& scheduler);

    ThreadPoolExecutor(ThreadPoolExecutor const&) = delete;
    ThreadPoolExecutor& operator=(ThreadPoolExecutor const&) = delete;

    void run() override;
    void finish() override;

    // IExecutor
    clock_type::time_point now() const override;
    void post(Task&&) override;
    std::shared_ptr<Cancelable> post_at(const clock_type::time_point& absTime, Task&&) override;
    std::shared_ptr<Cancelable> post_after(const clock_type::duration& relTime, Task&&) override;
    std::shared_ptr<Cancelable>
    post_periodically(const clock_type::duration& period, RepeatableTask&&) override;
    ISchedulerPtr get_scheduler() const override;

  private:
    ThreadPoolExecutor(const asyncly::ISchedulerPtr& scheduler);

  private:
    boost::mutex m_mutex;
    boost::condition_variable m_condition;
    unsigned int m_activeThreads;
    std::queue<Task> m_taskQueue;

    bool m_isShutdownActive;
    bool m_isStopped;

    const ISchedulerPtr m_scheduler;
};

template <typename Base>
std::shared_ptr<ThreadPoolExecutor<Base>>
ThreadPoolExecutor<Base>::create(const asyncly::ISchedulerPtr& scheduler)
{
    return std::shared_ptr<ThreadPoolExecutor>(new ThreadPoolExecutor(scheduler));
}

template <typename Base>
ThreadPoolExecutor<Base>::ThreadPoolExecutor(const asyncly::ISchedulerPtr& scheduler)
    : m_activeThreads(0)
    , m_isShutdownActive(false)
    , m_isStopped(false)
    , m_scheduler(scheduler)
{
}

template <typename Base> clock_type::time_point ThreadPoolExecutor<Base>::now() const
{
    return m_scheduler->now();
}

template <typename Base>
std::shared_ptr<Cancelable>
ThreadPoolExecutor<Base>::post_at(const clock_type::time_point& absTime, Task&& task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }
    task.maybe_set_executor(this->weak_from_this());
    return m_scheduler->execute_at(this->weak_from_this(), absTime, std::move(task));
}

template <typename Base>
std::shared_ptr<Cancelable>
ThreadPoolExecutor<Base>::post_after(const clock_type::duration& relTime, Task&& task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }
    task.maybe_set_executor(this->weak_from_this());
    return m_scheduler->execute_after(this->weak_from_this(), relTime, std::move(task));
}

template <typename Base>
std::shared_ptr<Cancelable> ThreadPoolExecutor<Base>::post_periodically(
    const clock_type::duration& period, RepeatableTask&& task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }
    return detail::PeriodicTask::create(period, std::move(task), this->shared_from_this());
}

template <typename Base> asyncly::ISchedulerPtr ThreadPoolExecutor<Base>::get_scheduler() const
{
    return m_scheduler;
}

template <typename Base> void ThreadPoolExecutor<Base>::post(Task&& closure)
{
    if (!closure) {
        throw std::runtime_error("invalid closure");
    }
    closure.maybe_set_executor(this->weak_from_this());
    {
        std::lock_guard<boost::mutex> lock(m_mutex);
        if (m_isStopped) {
            throw ExecutorStoppedException("executor stopped");
        }
        m_taskQueue.push(std::move(closure));
    }
    m_condition.notify_one();
}

template <typename Base> void ThreadPoolExecutor<Base>::finish()
{
    {
        std::lock_guard<boost::mutex> lock(m_mutex);
        m_isShutdownActive = true;
    }
    m_condition.notify_all();
}

template <typename Base> void ThreadPoolExecutor<Base>::run()
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
            return !m_taskQueue.empty() || m_isShutdownActive;
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
} // namespace asyncly
