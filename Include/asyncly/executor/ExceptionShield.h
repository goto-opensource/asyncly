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

#include <exception>
#include <functional>
#include <memory>
#include <queue>
#include <thread>
#include <utility>

#include "asyncly/executor/IExecutor.h"
#include "asyncly/task/detail/PeriodicTask.h"

namespace asyncly {

class ExceptionShield : public IExecutor, public std::enable_shared_from_this<ExceptionShield> {
  private:
    ExceptionShield(
        const std::shared_ptr<IExecutor>& executor,
        std::function<void(std::exception_ptr)> exceptionHandler);

  public:
    static std::shared_ptr<ExceptionShield> create(
        const std::shared_ptr<IExecutor>& executor,
        std::function<void(std::exception_ptr)> exceptionHandler);
    clock_type::time_point now() const override;
    void post(Task&& f) override;
    std::shared_ptr<Cancelable> post_at(const clock_type::time_point& t, Task&& f) override;
    std::shared_ptr<Cancelable> post_after(const clock_type::duration& t, Task&& f) override;
    std::shared_ptr<Cancelable>
    post_periodically(const clock_type::duration& t, CopyableTask task) override;
    ISchedulerPtr get_scheduler() const override;
    bool is_serializing() const override;

  private:
    const std::shared_ptr<IExecutor> executor_;
    const std::function<void(std::exception_ptr)> exceptionHandler_;

    // todo: replace by C++14 move-capture lambda
    class TaskHelper {
      public:
        TaskHelper(Task&& t, std::function<void(std::exception_ptr)> exceptionHandler)
            : task_(std::move(t))
            , exceptionHandler_(std::move(exceptionHandler))
        {
        }

        TaskHelper(TaskHelper&& o) = default;
        TaskHelper& operator=(TaskHelper&& o) = default;

        void operator()()
        {
            try {
                task_();
            } catch (...) {
                auto e = std::current_exception();
                exceptionHandler_(e);
            }
        }

      private:
        Task task_;
        std::function<void(std::exception_ptr)> exceptionHandler_;
    };
};

}
