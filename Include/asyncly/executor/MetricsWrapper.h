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

#include <memory>

#include <prometheus/collectable.h>
#include <prometheus/registry.h>

#include "asyncly/executor/IExecutor.h"
#include "asyncly/task/detail/PeriodicTask.h"

#include "detail//MetricsTaskState.h"
#include "detail/ExecutorMetrics.h"
#include "detail/MetricsTask.h"

namespace asyncly {

using IExecutorPtr = std::shared_ptr<IExecutor>;

///
/// A metric-collecting wrapper around an executor.
/// Creates metrics via prometheus-cpp.
///
/// ClockType must provide a function of the signature:
///  static clock_type::time_point now()
///
template <typename ClockType = clock_type>
class MetricsWrapper : public IExecutor,
                       public prometheus::Collectable,
                       public std::enable_shared_from_this<MetricsWrapper<ClockType>> {
  private:
    explicit MetricsWrapper(const IExecutorPtr& executor, const std::string& executorLabel = "");

  public:
    static std::shared_ptr<MetricsWrapper<ClockType>>
    create(const IExecutorPtr& executor, const std::string& executorLabel = "");

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
class MetricsCancelable : public Cancelable {
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

template <typename ClockType>
MetricsWrapper<ClockType>::MetricsWrapper(
    const IExecutorPtr& executor, const std::string& executorLabel)
    : executor_{ executor }
    , metrics_(std::make_shared<ExecutorMetrics>(executorLabel))
{
    if (!executor_) {
        throw std::runtime_error("must pass in non-null executor");
    }
}

template <typename ClockType>
std::shared_ptr<MetricsWrapper<ClockType>>
MetricsWrapper<ClockType>::create(const IExecutorPtr& executor, const std::string& executorLabel)
{
    return std::shared_ptr<MetricsWrapper<ClockType>>(
        new MetricsWrapper<ClockType>(executor, executorLabel));
}

template <typename ClockType> void MetricsWrapper<ClockType>::post(Task&& closure)
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

template <typename ClockType>
std::shared_ptr<Cancelable>
MetricsWrapper<ClockType>::post_at(const clock_type::time_point& t, Task&& closure)
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

template <typename ClockType>
std::shared_ptr<Cancelable>
MetricsWrapper<ClockType>::post_after(const clock_type::duration& t, Task&& closure)
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

template <typename ClockType>
std::shared_ptr<Cancelable>
MetricsWrapper<ClockType>::post_periodically(const clock_type::duration& period, CopyableTask task)
{
    return detail::PeriodicTask::create(period, std::move(task), this->shared_from_this());
}

template <typename ClockType>
std::shared_ptr<asyncly::IScheduler> MetricsWrapper<ClockType>::get_scheduler() const
{
    return executor_->get_scheduler();
}

template <typename ClockType> bool asyncly::MetricsWrapper<ClockType>::is_serializing() const
{
    return executor_->is_serializing();
}

template <typename ClockType>
std::vector<prometheus::MetricFamily> MetricsWrapper<ClockType>::Collect() const
{
    return metrics_->registry_.Collect();
}
}
