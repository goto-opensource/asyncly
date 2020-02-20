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

// Eric Niebler contributed this detection macro to Folly:
// https://github.com/facebook/folly/blob/master/folly/Portability.h#L407
#if defined(__cpp_coroutines) && __cpp_coroutines >= 201703L
#if defined(__has_include) && __has_include(<experimental/coroutine>)
#define ASYNCLY_HAS_COROUTINES 1
#endif
#endif

#if defined(_MSC_VER) && defined(_RESUMABLE_FUNCTIONS_SUPPORTED)
#define ASYNCLY_HAS_COROUTINES 1
#endif

// C++ Coroutine TS Trait implementations
#ifdef ASYNCLY_HAS_COROUTINES

#include <boost/optional.hpp>

#include <experimental/coroutine>

namespace asyncly {

template <typename T> class Future;
template <typename T> class Promise;

namespace detail {

template <typename T> struct coro_awaiter {
    coro_awaiter(Future<T>* future);
    ~coro_awaiter();
    bool await_ready();
    // todo: use bool-version to shortcut ready futures
    void await_suspend(std::experimental::coroutine_handle<> coroutine_handle);
    T await_resume();

  private:
    Future<T>* future_;
    std::exception_ptr error_;
    boost::optional<T> value_;
};

template <> struct coro_awaiter<void> {
    coro_awaiter(Future<void>* future);
    ~coro_awaiter();
    bool await_ready();
    // todo: use bool-version to shortcut ready futures
    void await_suspend(std::experimental::coroutine_handle<> coroutine_handle);
    void await_resume();

  private:
    Future<void>* future_;
    std::exception_ptr error_;
};

template <typename T> struct coro_promise {
    std::unique_ptr<Promise<T>> promise_;
    coro_promise();
    ~coro_promise();
    std::experimental::suspend_never initial_suspend();
    std::experimental::suspend_never final_suspend();
    void unhandled_exception();
    void return_value(T value);
};

template <> struct coro_promise<void> {
    std::unique_ptr<Promise<void>> promise_;
    coro_promise();
    ~coro_promise();
    Future<void> get_return_object();
    std::experimental::suspend_never initial_suspend();
    std::experimental::suspend_never final_suspend();
    void unhandled_exception();
    void return_void();
};
}
}

#endif
