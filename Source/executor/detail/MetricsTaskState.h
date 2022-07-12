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

#include <mutex>

#include <prometheus/counter.h>
#include <prometheus/gauge.h>

#include "asyncly/executor/detail/ExecutorMetrics.h"

namespace asyncly::detail {

/// MetricsTaskState offers thread safe synchronization about the state of timer tasks
/// needed for deciding whether tasks have been executed already when cancelled. This
/// is necessary for counting currently waiting tasks correctly in metrics.
class MetricsTaskState {
  public:
    MetricsTaskState(
        ExecutorMetricsPtr metrics,
        prometheus::Gauge& enqueuedTasks,
        prometheus::Counter& processedTasks);

    void onTaskExecutionStarted();
    void onTaskCancelled();

  private:
    std::mutex mutex_;
    ExecutorMetricsPtr metrics_;
    prometheus::Gauge& enqueuedTasks_;
    prometheus::Counter& processedTasks_;
    bool hasRun_ = false;
    bool wasCancelled_ = false;
};
} // namespace asyncly::detail
