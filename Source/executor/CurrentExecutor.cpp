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

#include "asyncly/executor/CurrentExecutor.h"

#include <stdexcept>

namespace asyncly {
namespace detail {

namespace {
thread_local asyncly::detail::ICurrentExecutorWrapper* current_executor_wrapper_rawptr_ = nullptr;
thread_local std::weak_ptr<asyncly::IExecutor> current_executor_weakptr_;
} // namespace

asyncly::detail::ICurrentExecutorWrapper* _get_current_executor_wrapper_rawptr()
{
    return current_executor_wrapper_rawptr_;
}

void _set_current_executor_wrapper_rawptr(asyncly::detail::ICurrentExecutorWrapper* ptr)
{
    current_executor_wrapper_rawptr_ = ptr;
}

const asyncly::IExecutorPtr _get_current_executor_sharedptr()
{
    return current_executor_weakptr_.lock();
}

void _set_current_executor_weakptr(std::weak_ptr<asyncly::IExecutor> wptr)
{
    current_executor_weakptr_ = wptr;
}

}

namespace this_thread {
void set_current_executor(std::weak_ptr<IExecutor> executor)
{
    detail::_set_current_executor_weakptr(executor);
}

asyncly::IExecutorPtr get_current_executor()
{
    if (auto currentWrapper = detail::_get_current_executor_wrapper_rawptr()) {
        if (auto executor = currentWrapper->get_current_executor()) {
            return executor;
        }
    } else if (auto currentSharedPtr = detail::_get_current_executor_sharedptr()) {
        return currentSharedPtr;
    }
    throw std::runtime_error("current executor stale");
}
}
}
