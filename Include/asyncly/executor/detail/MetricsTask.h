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
#include "asyncly/task/Task.h"

#include "MetricsTaskState.h"

namespace asyncly {

class MetricsTask {
  public:
    MetricsTask(
        Task&& t,
        prometheus::Histogram& taskExecutionDuration,
        prometheus::Histogram& taskQueueingDelay,
        const std::shared_ptr<detail::MetricsTaskState>& taskState);

    MetricsTask(const MetricsTask&) = delete;
    MetricsTask(MetricsTask&& o) = default;

    MetricsTask& operator=(const MetricsTask&) = delete;
    MetricsTask& operator=(MetricsTask&& o) = delete; // due to reference type members

    void operator()();

    Task task_;
    prometheus::Histogram& taskExecutionDuration_;
    prometheus::Histogram& taskQueueingDelay_;
    const std::shared_ptr<detail::MetricsTaskState> taskState_;
    const clock_type::time_point postTimePoint_;
};
}
