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

namespace asyncly {

using IExecutorPtr = std::shared_ptr<class IExecutor>;
using IStrandPtr = std::shared_ptr<class IStrand>;

namespace detail {

class ICurrentExecutorWrapper {
  public:
    virtual ~ICurrentExecutorWrapper() = default;
    virtual asyncly::IExecutorPtr get_current_executor() = 0;
};

// This is optimized for the default / standard executors, not the adapters.
// The raw pointer variant comes with almost no overhead during task execution if the current
// executor isn't required: The raw pointer refers to the stack object (TaskCurrentExecutorGuard)
// and the IExecutorPtr is stored as const& to the Task's shared_ptr copy. The current
// executor that is permanently set for the whole thread lifetime(required to be able to
// called future.then() in non - executor context) is now stored as weak_ptr.It's not
// blocking the executor from being removed, and it cannot leak.

asyncly::detail::ICurrentExecutorWrapper* _get_current_executor_wrapper_rawptr();
void _set_current_executor_wrapper_rawptr(asyncly::detail::ICurrentExecutorWrapper* ptr);

const asyncly::IExecutorPtr _get_current_executor_sharedptr();
void _set_current_executor_weakptr(std::weak_ptr<asyncly::IExecutor> wptr);

}

namespace this_thread {

/// allows to set a weak_ptr as thread-local storage
/// required for executor adapters for other frameworks (proxy/impl, ECWorkerThreadExecutor)
void set_current_executor(std::weak_ptr<IExecutor> executor);
asyncly::IExecutorPtr get_current_executor();
asyncly::IStrandPtr get_current_strand();
}
}
