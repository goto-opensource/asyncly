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

#include <functional>
#include <memory>

#include "asyncly/executor/IExecutor.h"
#include "asyncly/scheduler/IScheduler.h"

namespace asyncly {

class InlineExecutor final : public IExecutor, public std::enable_shared_from_this<InlineExecutor> {
  private:
    InlineExecutor();

  public:
    static std::shared_ptr<InlineExecutor> create();

    clock_type::time_point now() const override;
    void post(Task&& closure) override;
    std::shared_ptr<Cancelable> post_at(const clock_type::time_point&, Task&&) override;
    std::shared_ptr<Cancelable> post_after(const clock_type::duration&, Task&&) override;
    [[nodiscard]] std::shared_ptr<AutoCancelable>
    post_periodically(const clock_type::duration&, RepeatableTask&&) override;
    ISchedulerPtr get_scheduler() const override;

  private:
    const ISchedulerPtr _scheduler;
};
} // namespace asyncly
