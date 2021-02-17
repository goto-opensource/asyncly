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

#include <memory>
#include <queue>
#include <thread>
#include <utility>

#include "asyncly/executor/IStrand.h"
#include "asyncly/executor/Strand.h"
#include "asyncly/task/detail/PeriodicTask.h"

#include "asyncly/executor/ExceptionShield.h"

namespace asyncly {

namespace {
auto createTaskExceptionHandler(
    Task&& task, std::function<void(std::exception_ptr)> exceptionHandler)
{
    return [task{ std::move(task) }, exceptionHandler{ std::move(exceptionHandler) }] {
        try {
            task();
        } catch (...) {
            exceptionHandler(std::current_exception());
        }
    };
}
}

template <typename Base>
class ExceptionShield final : public Base,
                              public std::enable_shared_from_this<ExceptionShield<Base>> {
  public:
    ExceptionShield(
        const std::shared_ptr<IExecutor>& executor,
        std::function<void(std::exception_ptr)> exceptionHandler);

    clock_type::time_point now() const override;
    void post(Task&& f) override;
    std::shared_ptr<Cancelable> post_at(const clock_type::time_point& t, Task&& f) override;
    std::shared_ptr<Cancelable> post_after(const clock_type::duration& t, Task&& f) override;
    std::shared_ptr<Cancelable>
    post_periodically(const clock_type::duration& t, RepeatableTask&& task) override;
    ISchedulerPtr get_scheduler() const override;

  private:
    const std::shared_ptr<IExecutor> executor_;
    const std::function<void(std::exception_ptr)> exceptionHandler_;
};

template <typename Base>
ExceptionShield<Base>::ExceptionShield(
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

template <typename Base> clock_type::time_point ExceptionShield<Base>::now() const
{
    return executor_->now();
}

template <typename Base> void ExceptionShield<Base>::post(Task&& closure)
{
    closure.maybe_set_executor(this->shared_from_this());
    executor_->post(createTaskExceptionHandler(std::move(closure), exceptionHandler_));
}

template <typename Base>
std::shared_ptr<Cancelable>
ExceptionShield<Base>::post_at(const clock_type::time_point& t, Task&& closure)
{
    closure.maybe_set_executor(this->shared_from_this());
    return executor_->post_at(t, createTaskExceptionHandler(std::move(closure), exceptionHandler_));
}

template <typename Base>
std::shared_ptr<Cancelable>
ExceptionShield<Base>::post_after(const clock_type::duration& t, Task&& closure)
{
    closure.maybe_set_executor(this->shared_from_this());
    return executor_->post_after(
        t, createTaskExceptionHandler(std::move(closure), exceptionHandler_));
}

template <typename Base>
std::shared_ptr<Cancelable>
ExceptionShield<Base>::post_periodically(const clock_type::duration& period, RepeatableTask&& task)
{
    return detail::PeriodicTask::create(period, std::move(task), this->shared_from_this());
}

template <typename Base>
std::shared_ptr<asyncly::IScheduler> ExceptionShield<Base>::get_scheduler() const
{
    return executor_->get_scheduler();
}

IExecutorPtr create_exception_shield(
    const IExecutorPtr& executor, std::function<void(std::exception_ptr)> exceptionHandler)
{
    if (!executor) {
        throw std::runtime_error("must pass in non-null executor");
    }

    if (is_serializing(executor)) {
        return std::make_shared<ExceptionShield<IStrand>>(executor, exceptionHandler);
    } else {
        return std::make_shared<ExceptionShield<IExecutor>>(executor, exceptionHandler);
    }
}

}
