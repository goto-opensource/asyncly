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

#include "MetricsTask.h"
#include "asyncly/executor/IExecutor.h"

namespace asyncly {

MetricsTask::MetricsTask(
    Task&& t,
    const IExecutorPtr& executor,
    prometheus::Histogram& taskExecutionDuration,
    prometheus::Histogram& taskQueueingDelay,
    const std::shared_ptr<detail::MetricsTaskState>& taskState)
    : task_(std::move(t))
    , executor_(executor)
    , taskExecutionDuration_{ taskExecutionDuration }
    , taskQueueingDelay_{ taskQueueingDelay }
    , taskState_{ taskState }
    , postTimePoint_{ executor_->now() }
{
}

void MetricsTask::operator()()
{
    // Note: there is a short race condition here leading to wrong monitoring.
    // If the task is cancelled in between `onTaskExecutionStarted` and the invocation of the task
    // below, it is incorrectly counted as processed, even though it is skipped.
    taskState_->onTaskExecutionStarted();

    const auto start = executor_->now();
    taskQueueingDelay_.Observe(
        std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(start - postTimePoint_)
            .count());

    task_();

    taskExecutionDuration_.Observe(
        std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(
            executor_->now() - start)
            .count());
}

} // namespace asyncly