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

#include "asyncly/executor/detail/MetricsTask.h"

namespace asyncly {

MetricsTask::MetricsTask(
    Task&& t,
    prometheus::Histogram& taskExecutionDuration,
    prometheus::Histogram& taskQueueingDelay,
    const std::shared_ptr<detail::MetricsTaskState>& taskState)
    : task_(std::move(t))
    , taskExecutionDuration_{ taskExecutionDuration }
    , taskQueueingDelay_{ taskQueueingDelay }
    , taskState_{ taskState }
    , postTimePoint_{ clock_type::now() }
{
}

void MetricsTask::operator()()
{
    taskState_->onTaskExecutionStarted();
    const auto start = clock_type::now();
    taskQueueingDelay_.Observe(
        std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(start - postTimePoint_)
            .count());

    task_();

    taskExecutionDuration_.Observe(
        std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(
            clock_type::now() - start)
            .count());
}

}