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
    const ExecutorMetricsPtr& metrics,
    MetricsTask::ExecutionType executionType)
    : task_(std::move(t))
    , executor_(executor)
    , metrics_(metrics)
    , taskExecutionDuration_{ executionType == ExecutionType::timed
                                  ? metrics_->taskExecution.timed_
                                  : metrics_->taskExecution.immediate_ }
    , taskQueueingDelay_{ executionType == ExecutionType::timed ? metrics_->taskDelay.timed_
                                                                : metrics_->taskDelay.immediate_ }
    , enqueuedTasks_{ executionType == ExecutionType::timed ? metrics_->queuedTasks.timed_
                                                            : metrics_->queuedTasks.immediate_ }
    , processedTasks_{ executionType == ExecutionType::timed ? metrics_->processedTasks.timed_
                                                             : metrics_->processedTasks.immediate_ }
    , postTimePoint_{ executor_->now() }
{
}

void MetricsTask::operator()()
{
    enqueuedTasks_.Decrement();

    const auto start = executor_->now();
    taskQueueingDelay_.Observe(
        std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(start - postTimePoint_)
            .count());

    task_();
    processedTasks_.Increment();

    taskExecutionDuration_.Observe(
        std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(
            executor_->now() - start)
            .count());
}

} // namespace asyncly