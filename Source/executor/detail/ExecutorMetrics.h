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

#include <algorithm>
#include <iterator>
#include <vector>

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/metric_family.h>
#include <prometheus/registry.h>

namespace asyncly {

struct ProcessedTasksMetrics {
    ProcessedTasksMetrics(prometheus::Registry& registry, const std::string& executorLabel = "");

  private:
    prometheus::Family<prometheus::Counter>& family_;

  public:
    prometheus::Counter& immediate_;
    prometheus::Counter& timed_;
};

struct EnqueuedTasksMetrics {
    EnqueuedTasksMetrics(prometheus::Registry& registry, const std::string& executorLabel = "");

  private:
    prometheus::Family<prometheus::Gauge>& family_;

  public:
    prometheus::Gauge& immediate_;
    prometheus::Gauge& timed_;
};

struct TaskExecutionDurationMetrics {
    TaskExecutionDurationMetrics(
        prometheus::Registry& registry, const std::string& executorLabel = "");

  private:
    prometheus::Family<prometheus::Histogram>& family_;

  public:
    prometheus::Histogram& immediate_;
    prometheus::Histogram& timed_;
};

struct TaskQueueingDelayMetrics {
    TaskQueueingDelayMetrics(prometheus::Registry& registry, const std::string& executorLabel = "");

  private:
    prometheus::Family<prometheus::Histogram>& family_;

  public:
    prometheus::Histogram& immediate_;
    prometheus::Histogram& timed_;
};

struct ExecutorMetrics {
    ExecutorMetrics(const std::string& executorLabel = "");

    prometheus::Registry registry_;
    ProcessedTasksMetrics processedTasks;
    EnqueuedTasksMetrics queuedTasks;
    TaskExecutionDurationMetrics taskExecution;
    TaskQueueingDelayMetrics taskDelay;
};

using ExecutorMetricsPtr = std::shared_ptr<ExecutorMetrics>;
}
