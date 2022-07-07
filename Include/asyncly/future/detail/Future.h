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

#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <type_traits>
#include <variant>

#include <function2/function2.hpp>

#include "asyncly/future/detail/Coroutine.h"

#include "asyncly/detail/TypeUtils.h"

namespace asyncly {
template <typename T> class Future;
template <typename T> class Promise;

namespace detail {
template <typename T> class FutureImpl;
template <typename T> class PromiseImpl;
template <typename T>
std::tuple<std::shared_ptr<FutureImpl<T>>, std::shared_ptr<PromiseImpl<T>>> make_lazy_future_impl();
template <typename T>
std::shared_ptr<FutureImpl<typename std::decay<T>::type>> make_ready_future_impl(T&&);
inline std::shared_ptr<FutureImpl<void>> make_ready_future_impl();
template <typename T>
std::shared_ptr<FutureImpl<T>> make_exceptional_future_impl(std::exception_ptr e);

template <typename T> class PromiseImplBase {
  public:
    PromiseImplBase(const std::shared_ptr<FutureImpl<T>>& future);

    void set_exception(std::exception_ptr e);
    std::shared_ptr<FutureImpl<T>> get_future();

  protected:
    const std::shared_ptr<FutureImpl<T>> future_;
};

template <typename T> class PromiseImpl : public PromiseImplBase<T> {
  public:
    friend std::tuple<std::shared_ptr<FutureImpl<T>>, std::shared_ptr<PromiseImpl<T>>>
    make_lazy_future_impl<T>();

    PromiseImpl(const std::shared_ptr<FutureImpl<T>>&);

    void set_value(const T&);
    void set_value(T&&);
};

template <> class PromiseImpl<void> : public PromiseImplBase<void> {
  public:
    friend std::tuple<std::shared_ptr<FutureImpl<void>>, std::shared_ptr<PromiseImpl<void>>>
    make_lazy_future_impl<void>();

    PromiseImpl(const std::shared_ptr<FutureImpl<void>>&);

    void set_value();
};

struct ErrorSink {
    virtual ~ErrorSink() = default;
    virtual void notify_error_ready(std::exception_ptr) = 0;
};

template <typename T> struct resolve_handler {
    using type = fu2::unique_function<void(T)>;
};
template <> struct resolve_handler<void> {
    using type = fu2::unique_function<void()>;
};
template <typename T> using resolve_handler_t = typename resolve_handler<T>::type;
using reject_handler_t = fu2::unique_function<void(std::exception_ptr)>;

///    --> Resolved --
///    |             |
/// Ready -----------+-> Continued
///    |             |
///    --> Rejected --

namespace future_state {
template <typename T> struct Ready {
    resolve_handler_t<T> continuation_;
    reject_handler_t onError_;
    std::weak_ptr<ErrorSink> errorObserver_;
};

template <typename T> struct Resolved {
    T value_;
    void callContinuation(resolve_handler_t<T>& continuation);
};

template <> struct Resolved<void> {
    void callContinuation(resolve_handler_t<void>& continuation);
};

struct Rejected {
    std::exception_ptr error_;
};

struct Continued { };
} // namespace future_state

template <typename T> class FutureImplBase : public ErrorSink {
  public:
    template <typename F>
    typename std::shared_ptr<FutureImpl<continuation_future_element_type<T, F>>>
    then(F&& continuation);
    template <typename F> void catch_error(F&& f);
    template <typename F> void catch_and_forward_error(F&& f);

  protected:
    FutureImplBase();

  public:
    void notify_error_ready(std::exception_ptr) override;

  protected:
    std::variant<
        future_state::Ready<T>,
        future_state::Resolved<T>,
        future_state::Rejected,
        future_state::Continued>
        state_;

    bool continuationSet_;
    bool onErrorSet_;
    bool errorBreaksContinuationChain_ = true;

    std::mutex mutex_;
};

template <typename T> class FutureImpl : public FutureImplBase<T> {
  public:
    friend std::shared_ptr<FutureImpl<T>> make_ready_future_impl<T>(T&& value);
    friend std::shared_ptr<FutureImpl<T>> make_exceptional_future_impl<T>(std::exception_ptr e);
    friend std::tuple<std::shared_ptr<FutureImpl<T>>, std::shared_ptr<PromiseImpl<T>>>
    make_lazy_future_impl<T>();
    friend class PromiseImpl<T>;

    using value_type = T;

  private:
    // called by Promise<T>
    void notify_value_ready(const T& value);
    void notify_value_ready(T&& value);
};

template <> class FutureImpl<void> : public FutureImplBase<void> {
  public:
    friend std::shared_ptr<FutureImpl<void>> make_ready_future_impl();
    friend std::shared_ptr<FutureImpl<void>>
    make_exceptional_future_impl<void>(std::exception_ptr e);
    friend std::tuple<std::shared_ptr<FutureImpl<void>>, std::shared_ptr<PromiseImpl<void>>>
    make_lazy_future_impl<void>();
    friend class PromiseImpl<void>;

    using value_type = void;

  private:
    // called by Promise<T>
    void notify_value_ready();
};
} // namespace detail
} // namespace asyncly
