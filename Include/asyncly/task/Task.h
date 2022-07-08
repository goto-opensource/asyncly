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

#include "detail/TaskConcept.h"
#include "detail/TaskCurrentExecutorGuard.h"
#include "detail/TaskWrapper.h"

#include <memory>
#include <type_traits>

namespace asyncly {

struct Task {
    template <typename T, typename = typename std::enable_if<!std::is_same_v<T, Task>>>
    Task(T&& closure)
        : task_{ new detail::TaskWrapper<typename std::remove_reference<T>::type>{
            std::forward<T>(closure) } }
    {
    }

    template <typename T, typename = typename std::enable_if<!std::is_same_v<T, Task>>>
    Task(const T& closure)
        : task_{ new detail::TaskWrapper<T>{ closure } }
    {
    }

    void operator()() const
    {
        detail::TaskCurrentExecutorGuard guard(executor_);
        task_->run();
        task_.reset();
    }

    explicit operator bool() const
    {
        return task_ && *task_;
    }

    /// maybe_set_executor should be called by each executor in the
    /// stack the task is posted to. It will eventually make the
    /// original executor available inside the task by calling the
    /// global current_executor() function. Only the first executor
    /// (the top of the executor stack) is actually saved, all others
    /// are ignored. Executors can choose to ignore this (and not call
    /// this method) when it would not make sense (decorators meant to
    /// be used only once would be an example).
    void maybe_set_executor(const std::weak_ptr<IExecutor>& executor)
    {
        if (isExecutorSet_) {
            return;
        }
        isExecutorSet_ = true;

        executor_ = executor;
    }

    // order is important here. task_ including payload needs to be destroyed
    // before the executor is released
    std::weak_ptr<IExecutor> executor_;
    bool isExecutorSet_ = false;
    mutable std::unique_ptr<detail::TaskConcept> task_;
};

} // namespace asyncly
