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

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/metric_family.h>
#include <prometheus/registry.h>

#include "asyncly/ExecutorTypes.h"
#include "asyncly/executor/detail/ExecutorMetrics.h"
#include "asyncly/task/Task.h"

namespace asyncly {

class MetricsTask {
  public:
    enum class ExecutionType { timed, immediate };

    MetricsTask(
        Task&& t,
        const IExecutorPtr& executor,
        const ExecutorMetricsPtr& metrics,
        ExecutionType executionType);

    MetricsTask(const MetricsTask&) = delete;
    MetricsTask(MetricsTask&& o) = default;

    MetricsTask& operator=(const MetricsTask&) = delete;
    MetricsTask& operator=(MetricsTask&& o) = delete; // due to reference type members

    void operator()();

    Task task_;

    // reference is correct here since execution can only occur in an existing executor
    const IExecutorPtr& executor_;
    const ExecutorMetricsPtr metrics_;

    prometheus::Histogram& taskExecutionDuration_;
    prometheus::Histogram& taskQueueingDelay_;
    prometheus::Gauge& enqueuedTasks_;
    prometheus::Counter& processedTasks_;

    const clock_type::time_point postTimePoint_;
};
} // namespace asyncly
