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

#include "asyncly/executor/IStrand.h"
#include "asyncly/executor/MetricsWrapper.h"
#include "asyncly/executor/Strand.h"
#include "asyncly/executor/detail/ExecutorMetrics.h"
#include "asyncly/task/detail/PeriodicTask.h"

#include "detail/MetricsTask.h"

namespace asyncly {

///
/// A metric-collecting wrapper around an executor.
/// Creates metrics via prometheus-cpp.
///
template <typename Base>
class MetricsWrapper final : public Base,
                             public std::enable_shared_from_this<MetricsWrapper<Base>> {
  public:
    explicit MetricsWrapper(
        const IExecutorPtr& executor,
        const std::string& executorLabel,
        const std::shared_ptr<prometheus::Registry>& registry);

    clock_type::time_point now() const override
    {
        return executor_->now();
    }
    void post(Task&& f) override;
    std::shared_ptr<Cancelable> post_at(const clock_type::time_point& t, Task&& f) override;
    std::shared_ptr<Cancelable> post_after(const clock_type::duration& t, Task&& f) override;
    std::shared_ptr<AutoCancelable>
    post_periodically(const clock_type::duration& t, RepeatableTask&& f) override;
    ISchedulerPtr get_scheduler() const override;

  private:
    const IExecutorPtr executor_;
    const ExecutorMetricsPtr metrics_;
};

namespace {
class MetricsCancelable final : public Cancelable {
  public:
    MetricsCancelable(
        std::shared_ptr<Cancelable> cancelable,
        ExecutorMetricsPtr metrics,
        MetricsTask::ExecutionType executionType)
        : cancelable_{ cancelable }
        , metrics_(metrics)
        , enqueuedTasks_{ executionType == MetricsTask::ExecutionType::timed
                              ? metrics_->queuedTasks.timed_
                              : metrics_->queuedTasks.immediate_ }
    {
    }

    bool cancel() override
    {
        if (cancelable_->cancel()) {
            enqueuedTasks_.Decrement();
            return true;
        } else {
            return false;
        }
    }

    const std::shared_ptr<Cancelable> cancelable_;
    const ExecutorMetricsPtr metrics_;
    prometheus::Gauge& enqueuedTasks_;
};
} // namespace

template <typename Base>
MetricsWrapper<Base>::MetricsWrapper(
    const IExecutorPtr& executor,
    const std::string& executorLabel,
    const std::shared_ptr<prometheus::Registry>& registry)
    : executor_{ executor }
    , metrics_(std::make_shared<ExecutorMetrics>(registry, executorLabel))
{
    if (!executor_) {
        throw std::runtime_error("must pass in non-null executor");
    }
}

template <typename Base> void MetricsWrapper<Base>::post(Task&& closure)
{
    closure.maybe_set_executor(this->weak_from_this());
    metrics_->queuedTasks.immediate_.Increment();
    executor_->post(MetricsTask{
        std::move(closure), executor_, metrics_, MetricsTask::ExecutionType::immediate });
}

template <typename Base>
std::shared_ptr<Cancelable>
MetricsWrapper<Base>::post_at(const clock_type::time_point& t, Task&& closure)
{
    closure.maybe_set_executor(this->weak_from_this());
    metrics_->queuedTasks.timed_.Increment();
    auto cancelable = executor_->post_at(
        t,
        MetricsTask{ std::move(closure), executor_, metrics_, MetricsTask::ExecutionType::timed });
    return std::make_shared<MetricsCancelable>(
        cancelable, metrics_, MetricsTask::ExecutionType::timed);
}

template <typename Base>
std::shared_ptr<Cancelable>
MetricsWrapper<Base>::post_after(const clock_type::duration& t, Task&& closure)
{
    closure.maybe_set_executor(this->weak_from_this());
    metrics_->queuedTasks.timed_.Increment();
    auto cancelable = executor_->post_after(
        t,
        MetricsTask{ std::move(closure), executor_, metrics_, MetricsTask::ExecutionType::timed });
    return std::make_shared<MetricsCancelable>(
        cancelable, metrics_, MetricsTask::ExecutionType::timed);
}

template <typename Base>
std::shared_ptr<AutoCancelable>
MetricsWrapper<Base>::post_periodically(const clock_type::duration& period, RepeatableTask&& task)
{
    return std::make_shared<AutoCancelable>(
        detail::PeriodicTask::create(period, std::move(task), this->shared_from_this()));
}

template <typename Base>
std::shared_ptr<asyncly::IScheduler> MetricsWrapper<Base>::get_scheduler() const
{
    return executor_->get_scheduler();
}

IExecutorPtr create_metrics_wrapper(
    const IExecutorPtr& executor,
    const std::string& executorLabel,
    const std::shared_ptr<prometheus::Registry>& registry)
{
    if (!executor) {
        throw std::runtime_error("must pass in non-null executor");
    }

    if (is_serializing(executor)) {
        return std::make_shared<MetricsWrapper<IStrand>>(executor, executorLabel, registry);
    } else {
        return std::make_shared<MetricsWrapper<IExecutor>>(executor, executorLabel, registry);
    }
}
} // namespace asyncly
