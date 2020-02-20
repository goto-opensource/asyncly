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

#include "asyncly/executor/ExceptionShield.h"

namespace asyncly {

ExceptionShield::ExceptionShield(
    const std::shared_ptr<IExecutor>& executor,
    std::function<void(std::exception_ptr)> exceptionHandler)
    : executor_{ executor }
    , exceptionHandler_{ exceptionHandler }
{
    if (!executor_) {
        throw std::runtime_error("must pass in non-null executor");
    }
    if (!exceptionHandler_) {
        throw std::runtime_error("must pass in non-null exception handler");
    }
}

std::shared_ptr<ExceptionShield> ExceptionShield::create(
    const std::shared_ptr<IExecutor>& executor,
    std::function<void(std::exception_ptr)> exceptionHandler)
{
    return std::shared_ptr<ExceptionShield>(new ExceptionShield(executor, exceptionHandler));
}

clock_type::time_point ExceptionShield::now() const
{
    return executor_->now();
}

void ExceptionShield::post(Task&& closure)
{
    closure.maybe_set_executor(shared_from_this());
    executor_->post(ExceptionShield::TaskHelper{ std::move(closure), exceptionHandler_ });
}

std::shared_ptr<Cancelable>
ExceptionShield::post_at(const clock_type::time_point& t, Task&& closure)
{
    closure.maybe_set_executor(shared_from_this());
    return executor_->post_at(
        t, ExceptionShield::TaskHelper{ std::move(closure), exceptionHandler_ });
}

std::shared_ptr<Cancelable>
ExceptionShield::post_after(const clock_type::duration& t, Task&& closure)
{
    closure.maybe_set_executor(shared_from_this());
    return executor_->post_after(
        t, ExceptionShield::TaskHelper{ std::move(closure), exceptionHandler_ });
}

std::shared_ptr<Cancelable>
ExceptionShield::post_periodically(const clock_type::duration& period, CopyableTask task)
{
    return detail::PeriodicTask::create(period, std::move(task), shared_from_this());
}

std::shared_ptr<asyncly::IScheduler> ExceptionShield::get_scheduler() const
{
    return executor_->get_scheduler();
}

}