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

#include "asyncly/executor/IExecutor.h"

#include <mutex>

namespace asyncly::detail {

/**
 * Implementation of a periodic task, using the IExecutor framework. Until cancelled, it will
 * re-post itself. Construction is via create(), as we need to allocate and then schedule.
 */
class PeriodicTask : public Cancelable, public std::enable_shared_from_this<PeriodicTask> {
  private:
    // Token to prevent construction from outside create(), but still support std::make_shared
    struct Token {
    };

  public:
    // Due to the constructor wanting to schedule, and scheduling relying upon weak_from_this(), we
    // need a creator
    static std::shared_ptr<Cancelable>
    create(const clock_type::duration& period, RepeatableTask&& task, const IExecutorPtr& executor);

  public:
    void cancel() override;

  public:
    PeriodicTask(
        const clock_type::duration& period,
        RepeatableTask&& task,
        const IExecutorPtr& executor,
        Token token);

  private:
    void scheduleTask_();
    void onTimer_();

  private:
    std::mutex mutex_;
    const clock_type::duration period_;
    std::shared_ptr<RepeatableTask> task_;
    const std::weak_ptr<IExecutor> executor_;

    bool cancelled_;
    std::shared_ptr<Cancelable> currentDelayedTask_;
    clock_type::time_point expiry_;
};

} // namespace asyncly::detail
