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

#include "asyncly/task/Cancelable.h"
#include "asyncly/task/Task.h"

#include <memory>
#include <mutex>

namespace asyncly {

class TaskCancelable : public Cancelable {
  public:
    TaskCancelable(std::weak_ptr<Task>&& task)
        : isCanceled_(false)
        , isRunning_(false)
        , task_(std::move(task))
    {
    }

    void cancel() override
    {
        std::lock_guard<std::mutex> lock(mtx_);
        isCanceled_ = true;
        auto task = task_.lock();
        if (task) {
            task->executor_.reset();

            if (!isRunning_) {
                task->task_.reset();
            }
        }
    }

    bool maybeMarkAsRunning()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        isRunning_ = !isCanceled_;
        return isRunning_;
    }

  private:
    bool isCanceled_;
    bool isRunning_;
    std::weak_ptr<Task> task_;
    std::mutex mtx_;
};

} // namespace asyncly
