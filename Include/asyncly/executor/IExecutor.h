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

#include <chrono>
#include <memory>

#include "asyncly/ExecutorTypes.h"
#include "asyncly/executor/ISteadyClock.h"
#include "asyncly/task/AutoCancelable.h"
#include "asyncly/task/Cancelable.h"
#include "asyncly/task/RepeatableTask.h"
#include "asyncly/task/Task.h"

namespace asyncly {
class IExecutor : public ISteadyClock {
  public:
    virtual ~IExecutor() = default;
    virtual void post(Task&&) = 0;
    virtual std::shared_ptr<Cancelable> post_at(const clock_type::time_point& absTime, Task&&) = 0;
    virtual std::shared_ptr<Cancelable> post_after(const clock_type::duration& relTime, Task&&) = 0;
    [[nodiscard]] virtual std::shared_ptr<AutoCancelable>
    post_periodically(const clock_type::duration& period, RepeatableTask&&) = 0;
    virtual ISchedulerPtr get_scheduler() const = 0;
};
using IExecutorPtr = std::shared_ptr<IExecutor>;
} // namespace asyncly
