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

// C++ Coroutine TS Trait implementations
#ifdef ASYNCLY_HAS_COROUTINES

#include <experimental/coroutine>

namespace asyncly {
namespace detail {

template <typename T>
coro_awaiter<T>::coro_awaiter(Future<T>* future)
    : future_{ future }
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_awaiter::coro_awaiter"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
}

template <typename T> coro_awaiter<T>::~coro_awaiter()
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_awaiter::~coro_awaiter"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
}

template <typename T> bool coro_awaiter<T>::await_ready()
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "await_ready()"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
    return false;
}

// todo: use bool-version to shortcut ready futures
template <typename T>
void coro_awaiter<T>::await_suspend(std::experimental::coroutine_handle<> coroutine_handle)
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "await_suspend"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
    auto executor = this_thread::get_current_executor();
    future_
        ->catch_error([this, coroutine_handle, executor](auto e) mutable {
            error_ = e;
#ifdef ASYNCLY_FUTURE_DEBUG
            std::cerr << "catch_error: coroutine_handle.resume() on thread "
                      << std::this_thread::get_id() << std::endl;
#endif
            executor->post([coroutine_handle]() mutable { coroutine_handle.resume(); });
        })
        .then([this, coroutine_handle, executor](T value) mutable {
#ifdef ASYNCLY_FUTURE_DEBUG
            std::cerr << "then: coroutine_handle.resume() with value " << value << " on thread "
                      << std::this_thread::get_id() << std::endl;
#endif
            value_.emplace(value);
            executor->post([coroutine_handle]() mutable { coroutine_handle.resume(); });
        });
}

template <typename T> T coro_awaiter<T>::await_resume()
{
    if (error_) {
#ifdef ASYNCLY_FUTURE_DEBUG
        std::cerr << "await_resume with error on thread " << std::this_thread::get_id()
                  << std::endl;
#endif
        std::rethrow_exception(error_);
    } else {
        auto value = *value_;
#ifdef ASYNCLY_FUTURE_DEBUG
        std::cerr << "await_resume with value " << value << " on thread "
                  << std::this_thread::get_id() << std::endl;
#endif
        return value;
    }
}

inline coro_awaiter<void>::coro_awaiter(Future<void>* future)
    : future_{ future }
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_awaiter::coro_awaiter"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
}

inline coro_awaiter<void>::~coro_awaiter()
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_awaiter::~coro_awaiter"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
}

inline bool coro_awaiter<void>::await_ready()
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "await_ready()"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
    return false;
}

// todo: use bool-version to shortcut ready futures
inline void
coro_awaiter<void>::await_suspend(std::experimental::coroutine_handle<> coroutine_handle)
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "await_suspend"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
    auto executor = this_thread::get_current_executor();
    future_
        ->catch_error([this, coroutine_handle, executor](auto e) mutable {
            error_ = e;
#ifdef ASYNCLY_FUTURE_DEBUG
            std::cerr << "catch_error: coroutine_handle.resume() on thread "
                      << std::this_thread::get_id() << std::endl;
#endif
            executor->post([coroutine_handle]() mutable { coroutine_handle.resume(); });
        })
        .then([coroutine_handle, executor]() mutable {
#ifdef ASYNCLY_FUTURE_DEBUG
            std::cerr << "then: coroutine_handle.resume() on thread " << std::this_thread::get_id()
                      << std::endl;
#endif
            executor->post([coroutine_handle]() mutable { coroutine_handle.resume(); });
        });
}

inline void coro_awaiter<void>::await_resume()
{
    if (error_) {
#ifdef ASYNCLY_FUTURE_DEBUG
        std::cerr << "await_resume with error on thread " << std::this_thread::get_id()
                  << std::endl;
#endif
        std::rethrow_exception(error_);
    } else {
#ifdef ASYNCLY_FUTURE_DEBUG
        std::cerr << "await_resume on thread " << std::this_thread::get_id() << std::endl;
#endif
        return;
    }
}

template <typename T> std::experimental::suspend_never coro_promise<T>::initial_suspend()
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "initial_suspend"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
    return {};
}

template <typename T> std::experimental::suspend_never coro_promise<T>::final_suspend()
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "final_suspend"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
    return {};
}

template <typename T>
coro_promise<T>::coro_promise()
    : promise_{ new Promise<T>(std::get<1>(make_lazy_future<T>())) }
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_promise::coro_promise"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
}

template <typename T> void coro_promise<T>::unhandled_exception()
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_promise::set_exception"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
    promise_->set_exception(std::current_exception());
}

template <typename T> void coro_promise<T>::return_value(T value)
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_promise::return_value"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
    promise_->set_value(value);
}

inline coro_promise<void>::coro_promise()
    : promise_{ new Promise<void>(std::get<1>(make_lazy_future<void>())) }
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_promise::coro_promise"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
}

inline coro_promise<void>::~coro_promise()
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_promise::~coro_promise"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
}

inline Future<void> coro_promise<void>::get_return_object()
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_promise::get_return_object"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
    return promise_->get_future();
}

std::experimental::suspend_never coro_promise<void>::initial_suspend()
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_promise::initial_suspend"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
    return {};
}

std::experimental::suspend_never coro_promise<void>::final_suspend()
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_promise::final_suspend"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
    return {};
}

void coro_promise<void>::unhandled_exception()
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_promise::set_exception"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
    promise_->set_exception(std::current_exception());
}

void coro_promise<void>::return_void()
{
#ifdef ASYNCLY_FUTURE_DEBUG
    std::cerr << "coro_promise::return_void"
              << " on thread " << std::this_thread::get_id() << std::endl;
#endif
    promise_->set_value();
}
}
}

#endif
