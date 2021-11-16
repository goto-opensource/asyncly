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

#include "asyncly/executor/detail/AsioExecutor.h"

namespace asyncly {

AsioExecutor::AsioExecutor(const ISchedulerPtr& scheduler)
    : work_(boost::asio::make_work_guard(ioContext_.get_executor()))
    , m_scheduler(scheduler)
{
}

AsioExecutor::~AsioExecutor()
{
    finish();
}

void AsioExecutor::run()
{
    ioContext_.run();
}

void AsioExecutor::finish()
{
    work_.reset();
}

boost::asio::io_context& AsioExecutor::get_io_context()
{
    return ioContext_;
}

clock_type::time_point AsioExecutor::now() const
{
    return m_scheduler->now();
}

void AsioExecutor::post(Task&& closure)
{
    if (!closure) {
        throw std::runtime_error("invalid closure");
    }

    closure.maybe_set_executor(weak_from_this());
    boost::asio::post(ioContext_.get_executor(), std::move(closure));
}

std::shared_ptr<asyncly::Cancelable>
AsioExecutor::post_at(const clock_type::time_point& absTime, Task&& task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }
    task.maybe_set_executor(weak_from_this());
    return m_scheduler->execute_at(weak_from_this(), absTime, std::move(task));
}

std::shared_ptr<asyncly::Cancelable>
AsioExecutor::post_after(const clock_type::duration& period, Task&& task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }
    task.maybe_set_executor(weak_from_this());
    return m_scheduler->execute_after(weak_from_this(), period, std::move(task));
}

std::shared_ptr<asyncly::Cancelable>
AsioExecutor::post_periodically(const clock_type::duration& relTime, RepeatableTask&& task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }
    return detail::PeriodicTask::create(relTime, std::move(task), shared_from_this());
}

std::shared_ptr<asyncly::IScheduler> AsioExecutor::get_scheduler() const
{
    return m_scheduler;
}

} // namespace asyncly
