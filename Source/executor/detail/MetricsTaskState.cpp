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

#include "MetricsTaskState.h"

namespace asyncly::detail {

MetricsTaskState::MetricsTaskState(
    ExecutorMetricsPtr metrics,
    prometheus::Gauge& enqueuedTasks,
    prometheus::Counter& processedTasks)
    : metrics_(std::move(metrics))
    , enqueuedTasks_{ enqueuedTasks }
    , processedTasks_{ processedTasks }
{
}

void MetricsTaskState::onTaskExecutionStarted()
{
    bool expectedDequeued = false;
    if (dequeued_.compare_exchange_strong(expectedDequeued, true)) {
        enqueuedTasks_.Decrement();
        processedTasks_.Increment();
    }
}

void MetricsTaskState::onTaskCancelled()
{
    bool expectedDequeued = false;
    if (dequeued_.compare_exchange_strong(expectedDequeued, true)) {
        enqueuedTasks_.Decrement();
    }
}

} // namespace asyncly::detail