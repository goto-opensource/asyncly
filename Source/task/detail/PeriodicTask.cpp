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

#include "asyncly/task/detail/PeriodicTask.h"
#include "asyncly/Wrap.h"

namespace asyncly::detail {

std::shared_ptr<Cancelable> PeriodicTask::create(
    const clock_type::duration& period, RepeatableTask&& task, const IExecutorPtr& executor)
{
    auto periodicTask
        = std::make_shared<PeriodicTask>(period, std::move(task), executor, PeriodicTask::Token{});
    periodicTask->scheduleTask_();
    return periodicTask;
}

PeriodicTask::PeriodicTask(
    const clock_type::duration& period,
    RepeatableTask&& task,
    const IExecutorPtr& executor,
    PeriodicTask::Token)
    : period_(period)
    , task_(std::make_shared<RepeatableTask>(std::move(task)))
    , executor_(executor)
    , cancelled_(false)
    , expiry_(executor->now())
{
}

bool PeriodicTask::cancel()
{
    std::lock_guard<std::mutex> lock{ mutex_ };
    if (cancelled_) {
        return true;
    }
    cancelled_ = true;

    if (currentDelayedTask_) {
        currentDelayedTask_->cancel();
        currentDelayedTask_.reset();
    }

    task_.reset();
    return true;
}

void PeriodicTask::scheduleTask_()
{
    expiry_ += period_;
    if (auto executor = executor_.lock()) {
        currentDelayedTask_ = executor->post_at(
            expiry_, asyncly::wrap_weak_this_ignore(this, [this](auto) { onTimer_(); }));
    }
}

void PeriodicTask::onTimer_()
{
    // use a local copy to execute the task
    // => ensure that it is not destroyed during execution
    std::shared_ptr<RepeatableTask> task;
    {
        std::lock_guard<std::mutex> lock{ mutex_ };
        if (cancelled_) {
            return;
        }
        scheduleTask_();
        task = task_;
    }

    (*task)();
}

} // namespace asyncly::detail
