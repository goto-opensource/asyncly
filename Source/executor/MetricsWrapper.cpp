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

#include <memory>

#include <prometheus/registry.h>

#include "asyncly/executor/MetricsWrapper.h"
#include "asyncly/task/detail/PeriodicTask.h"

#include "detail/ExecutorMetrics.h"
#include "detail/MetricsTask.h"
#include "detail/MetricsTaskState.h"

namespace asyncly {

///
/// A metric-collecting wrapper around an executor.
/// Creates metrics via prometheus-cpp.
///

class MetricsWrapper final : public IExecutor,
                             public prometheus::Collectable,
                             public std::enable_shared_from_this<MetricsWrapper> {
  public:
    explicit MetricsWrapper(const IExecutorPtr& executor, const std::string& executorLabel);

    clock_type::time_point now() const override
    {
        return executor_->now();
    }
    void post(Task&& f) override;
    std::shared_ptr<Cancelable> post_at(const clock_type::time_point& t, Task&& f) override;
    std::shared_ptr<Cancelable> post_after(const clock_type::duration& t, Task&& f) override;
    std::shared_ptr<Cancelable>
    post_periodically(const clock_type::duration& t, CopyableTask f) override;
    ISchedulerPtr get_scheduler() const override;
    bool is_serializing() const override;

    std::vector<prometheus::MetricFamily> Collect() const override;

  private:
    const IExecutorPtr executor_;
    ExecutorMetricsPtr metrics_;
};

namespace {
class MetricsCancelable final : public Cancelable {
  public:
    MetricsCancelable(
        std::shared_ptr<Cancelable> cancelable,
        const std::shared_ptr<detail::MetricsTaskState>& taskState)
        : cancelable_{ cancelable }
        , taskState_{ taskState }
    {
    }

    void cancel() override
    {
        taskState_->onTaskCancelled();
        cancelable_->cancel();
    }

    const std::shared_ptr<Cancelable> cancelable_;
    const std::shared_ptr<detail::MetricsTaskState> taskState_;
};
}

MetricsWrapper::MetricsWrapper(const IExecutorPtr& executor, const std::string& executorLabel)
    : executor_{ executor }
    , metrics_(std::make_shared<ExecutorMetrics>(executorLabel))
{
    if (!executor_) {
        throw std::runtime_error("must pass in non-null executor");
    }
}

void MetricsWrapper::post(Task&& closure)
{
    closure.maybe_set_executor(this->shared_from_this());
    auto taskState = std::make_shared<detail::MetricsTaskState>(
        metrics_, metrics_->queuedTasks.immediate_, metrics_->processedTasks.immediate_);
    taskState->onTaskEnqueued();
    executor_->post(MetricsTask{ std::move(closure),
                                 metrics_->taskExecution.immediate_,
                                 metrics_->taskDelay.immediate_,
                                 taskState });
}

std::shared_ptr<Cancelable> MetricsWrapper::post_at(const clock_type::time_point& t, Task&& closure)
{
    closure.maybe_set_executor(this->shared_from_this());
    auto taskState = std::make_shared<detail::MetricsTaskState>(
        metrics_, metrics_->queuedTasks.timed_, metrics_->processedTasks.timed_);
    taskState->onTaskEnqueued();
    auto cancelable = executor_->post_at(
        t,
        MetricsTask{ std::move(closure),
                     metrics_->taskExecution.timed_,
                     metrics_->taskDelay.timed_,
                     taskState });
    return std::make_shared<MetricsCancelable>(cancelable, taskState);
}

std::shared_ptr<Cancelable>
MetricsWrapper::post_after(const clock_type::duration& t, Task&& closure)
{
    closure.maybe_set_executor(this->shared_from_this());
    auto taskState = std::make_shared<detail::MetricsTaskState>(
        metrics_, metrics_->queuedTasks.timed_, metrics_->processedTasks.timed_);
    taskState->onTaskEnqueued();
    auto cancelable = executor_->post_after(
        t,
        MetricsTask{ std::move(closure),
                     metrics_->taskExecution.timed_,
                     metrics_->taskDelay.timed_,
                     taskState });
    return std::make_shared<MetricsCancelable>(cancelable, taskState);
}

std::shared_ptr<Cancelable>
MetricsWrapper::post_periodically(const clock_type::duration& period, CopyableTask task)
{
    return detail::PeriodicTask::create(period, std::move(task), this->shared_from_this());
}

std::shared_ptr<asyncly::IScheduler> MetricsWrapper::get_scheduler() const
{
    return executor_->get_scheduler();
}

bool asyncly::MetricsWrapper::is_serializing() const
{
    return executor_->is_serializing();
}

std::vector<prometheus::MetricFamily> MetricsWrapper::Collect() const
{
    return metrics_->registry_.Collect();
}

std::pair<IExecutorPtr, std::shared_ptr<prometheus::Collectable>>
create_metrics_wrapper(const IExecutorPtr& executor, const std::string& executorLabel)
{
    if (!executor) {
        throw std::runtime_error("must pass in non-null executor");
    }

    auto metricsWrapper = std::make_shared<MetricsWrapper>(executor, executorLabel);
    return { metricsWrapper, metricsWrapper };
}
}
