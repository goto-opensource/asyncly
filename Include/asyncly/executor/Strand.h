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

#include <deque>
#include <future>
#include <memory>

#include "asyncly/executor/IExecutor.h"
#include "asyncly/scheduler/IScheduler.h"

namespace asyncly {

/// Strand implements a serializing queue used to dispatch tasks that have to be executed
/// sequentially.
class Strand : public IExecutor, public std::enable_shared_from_this<Strand> {
  public:
    /// Construct a new Strand.
    /// @param executor The underlying executor tasks are forwarded to.
    Strand(const IExecutorPtr& executor);

    ~Strand();

  public:
    /// get current time
    clock_type::time_point now() const override;

    /// post a task to the Strand. All tasks are guaranteed to not be executed in parallel.
    void post(Task&&) override;

    /// post a task to the underlying Strand at a given time point
    std::shared_ptr<Cancelable> post_at(const clock_type::time_point& absTime, Task&&) override;

    /// post a task to the underlying Strand after a given time period
    std::shared_ptr<Cancelable> post_after(const clock_type::duration& relTime, Task&&) override;

    /// post a task to the underlying Strand peridocially
    std::shared_ptr<Cancelable>
    post_periodically(const clock_type::duration& period, CopyableTask) override;

    ISchedulerPtr get_scheduler() const override;
    bool is_serializing() const override;

  private:
    enum class State {
        Waiting,
        Executing,
    };

    void notifyDone();

    const IExecutorPtr executor_;
    std::deque<Task> taskQueue_;
    std::mutex mutex_;
    State state_;
    std::promise<void> destroyed_;
    std::shared_future<void> destroyedFuture_;
};
}
