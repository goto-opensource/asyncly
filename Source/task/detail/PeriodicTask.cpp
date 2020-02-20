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

#include <cassert>

#include "asyncly/task/detail/PeriodicTask.h"

namespace asyncly {
namespace detail {

std::shared_ptr<Cancelable> PeriodicTask::create(
    const clock_type::duration& period, CopyableTask task, const IExecutorPtr& executor)
{
    if (task == nullptr) {
        throw std::runtime_error("invalid task");
    }

    auto periodicTask
        = std::make_shared<PeriodicTask>(period, std::move(task), executor, PeriodicTask::Token{});
    periodicTask->scheduleTask_();
    return periodicTask;
}

PeriodicTask::PeriodicTask(
    const clock_type::duration& period,
    CopyableTask task,
    const IExecutorPtr& executor,
    PeriodicTask::Token)
    : period_(period)
    , task_(std::move(task))
    , executor_(executor)
    , cancelled_(false)
    , expiry_(executor->now())
{
}

void PeriodicTask::cancel()
{
    std::lock_guard<std::mutex> lock{ mutex_ };
    cancel_();
}

void PeriodicTask::scheduleTask_()
{
    expiry_ += period_;
    if (auto executor = executor_.lock()) {
        currentDelayedTask_
            = executor->post_at(expiry_, [self = shared_from_this()]() { self->onTimer_(); });
    }
}

void PeriodicTask::cancel_()
{
    if (cancelled_) {
        return;
    }
    cancelled_ = true;
    cancelAndClean_();
}

void PeriodicTask::onTimer_()
{
    std::lock_guard<std::mutex> lock{ mutex_ };
    if (cancelled_) {
        return;
    }

    if (auto executor = executor_.lock()) {
        executor->post([self = shared_from_this()]() { self->execute(); });
    }

    scheduleTask_();
}

void PeriodicTask::cancelAndClean_()
{
    if (currentDelayedTask_) {
        currentDelayedTask_->cancel();
    }
    currentDelayedTask_.reset();

    task_ = nullptr;
}

void PeriodicTask::execute()
{
    if (cancelled_) {
        return;
    }

    task_();
}

} // detail namespace
} // executor namespace
