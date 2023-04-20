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

#include "asyncly/executor/detail/ExternalEventExecutor.h"

#include "asyncly/executor/ExecutorStoppedException.h"

namespace asyncly {

std::shared_ptr<ExternalEventExecutor> ExternalEventExecutor::create(
    const asyncly::ISchedulerPtr& scheduler, const ExternalEventFunction& externalEventFunction)
{
    auto executor = std::shared_ptr<ExternalEventExecutor>(
        new ExternalEventExecutor(scheduler, externalEventFunction));
    return executor;
}

ExternalEventExecutor::ExternalEventExecutor(
    const asyncly::ISchedulerPtr& scheduler, const ExternalEventFunction& externalEventFunction)
    : m_externalEventFunction(externalEventFunction)
    , m_isStopped(false)
    , m_scheduler(scheduler)
{
}

clock_type::time_point ExternalEventExecutor::now() const
{
    return m_scheduler->now();
}

std::shared_ptr<Cancelable>
ExternalEventExecutor::post_at(const clock_type::time_point& absTime, Task&& task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }
    task.maybe_set_executor(weak_from_this());
    return m_scheduler->execute_at(weak_from_this(), absTime, std::move(task));
}

std::shared_ptr<Cancelable>
ExternalEventExecutor::post_after(const clock_type::duration& relTime, Task&& task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }
    task.maybe_set_executor(weak_from_this());
    return m_scheduler->execute_after(weak_from_this(), relTime, std::move(task));
}

std::shared_ptr<AutoCancelable>
ExternalEventExecutor::post_periodically(const clock_type::duration& period, RepeatableTask&& task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }
    return std::make_shared<AutoCancelable>(
        detail::PeriodicTask::create(period, std::move(task), shared_from_this()));
}

asyncly::ISchedulerPtr ExternalEventExecutor::get_scheduler() const
{
    return m_scheduler;
}

void ExternalEventExecutor::post(Task&& closure)
{
    if (!closure) {
        throw std::runtime_error("invalid closure");
    }
    closure.maybe_set_executor(weak_from_this());
    bool signalExternalEvent = false;
    {
        std::lock_guard lock{ m_mutex };
        if (m_isStopped) {
            throw ExecutorStoppedException("executor stopped");
        }
        signalExternalEvent = m_taskQueue.empty();
        m_taskQueue.push(std::move(closure));
    }
    if (signalExternalEvent) {
        m_externalEventFunction();
    }
}

void ExternalEventExecutor::runOnce()
{
    while (true) {
        {
            std::lock_guard lock{ m_mutex };
            if (m_taskQueue.empty()) {
                return;
            }
            m_execQueue.swap(m_taskQueue);
        }

        while (!m_execQueue.empty()) {
            m_execQueue.front()();
            m_execQueue.pop();
        }
    }
}
} // namespace asyncly