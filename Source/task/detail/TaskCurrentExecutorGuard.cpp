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

#include "asyncly/task/detail/TaskCurrentExecutorGuard.h"

namespace asyncly {
namespace detail {
TaskCurrentExecutorGuard::TaskCurrentExecutorGuard(const std::weak_ptr<IExecutor>& executor)
    : executor_(executor)
{
    last_executor_wrapper_rawptr_ = _get_current_executor_wrapper_rawptr();
    _set_current_executor_wrapper_rawptr(this);
}

TaskCurrentExecutorGuard::~TaskCurrentExecutorGuard()
{
    _set_current_executor_wrapper_rawptr(last_executor_wrapper_rawptr_);
}

std::shared_ptr<asyncly::IExecutor> TaskCurrentExecutorGuard::get_current_executor()
{
    return executor_.lock();
}
}
}
