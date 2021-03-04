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
#include <future>
#include <memory>

#include "StrandImpl.h"
#include "asyncly/scheduler/IScheduler.h"
#include "asyncly/task/detail/PeriodicTask.h"

namespace asyncly {

StrandImpl::StrandImpl(const IExecutorPtr& executor)
    : executor_{ executor }
    , state_{ StrandImpl::State::Waiting }
    , destroyedFuture_{ destroyed_.get_future() }
{
}

StrandImpl::~StrandImpl()
{
    destroyed_.set_value();
}

clock_type::time_point StrandImpl::now() const
{
    return executor_->now();
}

void StrandImpl::post(Task&& task)
{
    if (!task) {
        throw std::runtime_error("invalid closure");
    }

    task.maybe_set_executor(weak_from_this());

    std::unique_lock<std::mutex> lock(mutex_);
    switch (state_) {
    case StrandImpl::State::Waiting:
        state_ = StrandImpl::State::Executing;
        lock.unlock();
        executor_->post([ctask = std::move(task), self = shared_from_this()]() {
            ctask();
            self->notifyDone();
        });
        break;

    case StrandImpl::State::Executing:
        taskQueue_.push_back(std::move(task));
        break;
    }
}

std::shared_ptr<asyncly::Cancelable>
StrandImpl::post_at(const clock_type::time_point& absTime, Task&& task)
{
    task.maybe_set_executor(weak_from_this());
    return get_scheduler()->execute_at(weak_from_this(), absTime, std::move(task));
}

std::shared_ptr<asyncly::Cancelable>
StrandImpl::post_after(const clock_type::duration& relTime, Task&& task)
{
    task.maybe_set_executor(weak_from_this());
    return get_scheduler()->execute_after(weak_from_this(), relTime, std::move(task));
}

std::shared_ptr<asyncly::Cancelable>
StrandImpl::post_periodically(const clock_type::duration& period, RepeatableTask&& task)
{
    return detail::PeriodicTask::create(period, std::move(task), shared_from_this());
}

ISchedulerPtr StrandImpl::get_scheduler() const
{
    return executor_->get_scheduler();
}

void StrandImpl::notifyDone()
{
    std::unique_lock<std::mutex> lock(mutex_);

    assert(state_ == StrandImpl::State::Executing);

    if (taskQueue_.empty()) {
        state_ = StrandImpl::State::Waiting;
        return;
    }

    auto task = std::move(taskQueue_.front());
    taskQueue_.pop_front();
    lock.unlock();

    executor_->post([ctask = std::move(task), self = shared_from_this()]() {
        ctask();
        self->notifyDone();
    });
}
}
