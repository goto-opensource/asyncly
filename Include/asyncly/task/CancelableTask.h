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

#include "Task.h"
#include "detail/TaskCancelable.h"

#include <memory>

namespace asyncly {

class CancelableTask {
  public:
    CancelableTask(
        const std::shared_ptr<Task>& task, const std::shared_ptr<TaskCancelable>& cancelable)
        : task_(task)
        , cancelable_(cancelable)
    {
    }

    CancelableTask(CancelableTask&& other) = default;

    void operator()()
    {
        if (cancelable_->maybeMarkAsRunning()) {
            (*task_)();
        }
    }

    void operator()() const
    {
        if (cancelable_->maybeMarkAsRunning()) {
            (*task_)();
        }
    }

  private:
    const std::shared_ptr<Task> task_;
    const std::shared_ptr<TaskCancelable> cancelable_;
};
}
