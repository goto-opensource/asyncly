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

#include "asyncly/executor/detail/ExecutorMetrics.h"

#include <iterator>

namespace asyncly {

namespace {
// creates buckets with exponential bucket sizes
inline std::vector<double> createDurationBuckets(std::size_t buckets, double start, double factor)
{
    std::vector<double> result;
    result.reserve(buckets);

    std::generate_n(std::back_inserter(result), buckets, [&start, &factor] {
        start = factor * start;
        return start;
    });

    return result;
}
} // namespace

ProcessedTasksMetrics::ProcessedTasksMetrics(
    prometheus::Registry& registry, const std::string& executorLabel)
    : family_{ prometheus::BuildCounter()
                   .Name("processed_tasks_total")
                   .Help("Number of tasks pulled out of the queue and run "
                         "by this executor.")
                   .Register(registry) }
    , immediate_{ family_.Add({ { "executor", executorLabel }, { "type", "immediate" } }) }
    , timed_{ family_.Add({ { "executor", executorLabel }, { "type", "timed" } }) }
{
}

EnqueuedTasksMetrics::EnqueuedTasksMetrics(
    prometheus::Registry& registry, const std::string& executorLabel)
    : family_{ prometheus::BuildGauge()
                   .Name("currently_enqueued_tasks_total")
                   .Help("Number of tasks currently residing in the executors "
                         "task queue and waiting to be executed.")
                   .Register(registry) }
    , immediate_{ family_.Add({ { "executor", executorLabel }, { "type", "immediate" } }) }
    , timed_{ family_.Add({ { "executor", executorLabel }, { "type", "timed" } }) }
{
}

TaskExecutionDurationMetrics::TaskExecutionDurationMetrics(
    prometheus::Registry& registry, const std::string& executorLabel)
    : family_{ prometheus::BuildHistogram()
                   .Name("task_execution_duration_ns")
                   .Help("Histogram of time taken for tasks to run once they "
                         "have been taken out of the queue and started.")
                   .Register(registry) }
    , immediate_{ family_.Add(
          { { "executor", executorLabel } },
          createDurationBuckets(12u, 250.0, 4.0) // 1us to 4s with exponential bucket sizes
          ) }

    , timed_{ family_.Add(
          { { "executor", executorLabel } },
          createDurationBuckets(12u, 250.0, 4.0) // 1us to 4s with exponential bucket sizes
          ) }
{
}

TaskQueueingDelayMetrics::TaskQueueingDelayMetrics(
    prometheus::Registry& registry, const std::string& executorLabel)
    : family_{ prometheus::BuildHistogram()
                   .Name("task_queueing_delay_ns")
                   .Help("Histogram of the queuing time of tasks, i.e., the time it takes from "
                         "their creation to their execution.")
                   .Register(registry) }
    , immediate_{ family_.Add(
          { { "executor", executorLabel }, { "type", "immediate" } },
          createDurationBuckets(15u, 250.0, 4.0) // 1us to 4m with exponential bucket sizes
          ) }
    , timed_{ family_.Add(
          { { "executor", executorLabel }, { "type", "timed" } },
          createDurationBuckets(15u, 250.0, 4.0) // 1us to 4m with exponential bucket sizes
          ) }
{
}

ExecutorMetrics::ExecutorMetrics(
    const std::shared_ptr<prometheus::Registry>& registry, const std::string& executorLabel)
    : registry_(registry)
    , processedTasks{ *registry_, executorLabel }
    , queuedTasks{ *registry_, executorLabel }
    , taskExecution{ *registry_, executorLabel }
    , taskDelay{ *registry_, executorLabel }
{
}
} // namespace asyncly
