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

#include <asyncly/executor/ExecutorStoppedException.h>
#include <asyncly/executor/IExecutor.h>
#include <asyncly/future/detail/Future.h>

#include <boost/core/enable_if.hpp>
#include <boost/hana/functional/overload.hpp>

#include <type_traits>

namespace asyncly::detail {

namespace {
template <typename T, typename D = void> struct maybe_pack_and_save;
template <typename T>
struct maybe_pack_and_save<
    T,
    typename boost::disable_if<typename is_tuple<typename std::decay<T>::type>::type>::type> {

    maybe_pack_and_save(std::shared_ptr<PromiseImpl<T>>& promise)
        : promise_{ promise }
    {
    }

    void operator()(T value)
    {
        promise_->set_value(std::move(value));
    }

    const std::shared_ptr<PromiseImpl<T>> promise_;
};

template <typename T>
struct maybe_pack_and_save<
    T,
    typename std::enable_if<is_tuple<typename std::decay<T>::type>::type::value>::type> {

    maybe_pack_and_save(std::shared_ptr<PromiseImpl<T>>& promise)
        : promise_{ promise }
    {
    }

    template <typename... Args> void operator()(Args&&... args)
    {
        promise_->set_value(std::make_tuple(std::forward<Args>(args)...));
    }

    const std::shared_ptr<PromiseImpl<T>> promise_;
};
} // namespace

template <typename T>
PromiseImplBase<T>::PromiseImplBase(const std::shared_ptr<FutureImpl<T>>& future)
    : future_{ future }
{
}

template <typename T> void PromiseImplBase<T>::set_exception(std::exception_ptr e)
{
    future_->notify_error_ready(e);
}

template <typename T> std::shared_ptr<FutureImpl<T>> PromiseImplBase<T>::get_future()
{
    return future_;
}

template <typename T>
PromiseImpl<T>::PromiseImpl(const std::shared_ptr<FutureImpl<T>>& future)
    : PromiseImplBase<T>{ future }
{
}

template <typename T> void PromiseImpl<T>::set_value(const T& value)
{
    this->future_->notify_value_ready(value);
}

template <typename T> void PromiseImpl<T>::set_value(T&& value)
{
    this->future_->notify_value_ready(std::forward<T>(value));
}

inline PromiseImpl<void>::PromiseImpl(const std::shared_ptr<FutureImpl<void>>& future)
    : PromiseImplBase<void>{ future }
{
}

inline void PromiseImpl<void>::set_value()
{
    future_->notify_value_ready();
}

template <typename T>
std::shared_ptr<FutureImpl<typename std::decay<T>::type>> make_ready_future_impl(T&& value)
{
    auto future = std::make_shared<FutureImpl<typename std::decay<T>::type>>();
    future->notify_value_ready(std::forward<T>(value));
    return future;
}

inline std::shared_ptr<FutureImpl<void>> make_ready_future_impl()
{
    auto future = std::make_shared<FutureImpl<void>>();
    future->notify_value_ready();
    return future;
}

template <typename T>
std::shared_ptr<FutureImpl<T>> make_exceptional_future_impl(std::exception_ptr e)
{
    auto future = std::make_shared<FutureImpl<T>>();
    future->notify_error_ready(e);
    return future;
}

template <typename T>
std::tuple<std::shared_ptr<FutureImpl<T>>, std::shared_ptr<PromiseImpl<T>>> make_lazy_future_impl()
{
    auto future = std::make_shared<FutureImpl<T>>();
    auto promise = std::make_shared<PromiseImpl<T>>(future);
    return std::make_tuple(future, promise);
}

namespace {

/// Binder classes for continuations, these can be replaced by move-capture lambdas in C++14
template <typename T, typename F, typename U> struct FutureVoidBinder {
    FutureVoidBinder(T value, F continuation, std::shared_ptr<PromiseImpl<U>> promise)
        : value_{ std::move(value) }
        , continuation_{ std::move(continuation) }
        , promise_{ std::move(promise) }
    {
    }

    FutureVoidBinder(const FutureVoidBinder<T, F, U>&) = delete;
    FutureVoidBinder(FutureVoidBinder<T, F, U>&& o) = default;

    void operator()()
    {
        auto promise = promise_;
        try {
            maybe_unpack_and_call<T>{}(continuation_, std::move(value_))
                .then([promise]() { promise->set_value(); })
                .catch_error([promise](std::exception_ptr e) { promise->set_exception(e); });
        } catch (...) {
            auto e = std::current_exception();
            promise->set_exception(e);
        }
    }

    T value_;
    static_assert(!std::is_reference<F>::value, "F must not be a reference type!");
    F continuation_;
    std::shared_ptr<PromiseImpl<U>> promise_;
};

template <typename F, typename U> struct FutureVoidBinder<void, F, U> {
    FutureVoidBinder(F continuation, std::shared_ptr<PromiseImpl<U>> promise)
        : continuation_{ std::move(continuation) }
        , promise_{ std::move(promise) }
    {
    }

    FutureVoidBinder(const FutureVoidBinder<void, F, U>&) = delete;
    FutureVoidBinder(FutureVoidBinder<void, F, U>&& o) = default;

    void operator()()
    {
        auto promise = promise_;
        try {
            continuation_()
                .then([promise]() { promise->set_value(); })
                .catch_error([promise](std::exception_ptr e) { promise->set_exception(e); });
        } catch (...) {
            auto e = std::current_exception();
            promise->set_exception(e);
            return;
        }
    }

    static_assert(!std::is_reference<F>::value, "F must not be a reference type!");
    F continuation_;
    std::shared_ptr<PromiseImpl<U>> promise_;
};

template <typename T, typename F, typename U> struct FutureValueBinder {
    FutureValueBinder(T value, F continuation, std::shared_ptr<PromiseImpl<U>> promise)
        : value_{ std::move(value) }
        , continuation_{ std::move(continuation) }
        , promise_{ std::move(promise) }
    {
    }

    FutureValueBinder(const FutureValueBinder<T, F, U>&) = delete;
    FutureValueBinder(FutureValueBinder<T, F, U>&& o) = default;

    void operator()()
    {
        auto promise = promise_;
        try {
            maybe_unpack_and_call<T>{}(continuation_, std::move(value_))
                .then([promise](U result) { promise->set_value(std::forward<U>(result)); })
                .catch_error([promise](std::exception_ptr e) { promise->set_exception(e); });
        } catch (...) {
            auto e = std::current_exception();
            promise->set_exception(e);
        }
    }

    T value_;
    static_assert(!std::is_reference<F>::value, "F must not be a reference type!");
    F continuation_;
    std::shared_ptr<PromiseImpl<U>> promise_;
};

template <typename F, typename U> struct FutureValueBinder<void, F, U> {
    FutureValueBinder(F continuation, std::shared_ptr<PromiseImpl<U>> promise)
        : continuation_{ std::move(continuation) }
        , promise_{ std::move(promise) }
    {
    }

    FutureValueBinder(const FutureValueBinder<void, F, U>&) = delete;
    FutureValueBinder(FutureValueBinder<void, F, U>&& o) = default;

    void operator()()
    {
        auto promise = promise_;
        try {
            continuation_()
                .then(maybe_pack_and_save<U>{ promise })
                .catch_error([promise](std::exception_ptr e) { promise->set_exception(e); });
        } catch (...) {
            auto e = std::current_exception();
            promise->set_exception(e);
        }
    }

    static_assert(!std::is_reference<F>::value, "F must not be a reference type!");
    F continuation_;
    std::shared_ptr<PromiseImpl<U>> promise_;
};

template <typename T, typename F, typename U> struct NonFutureVoidBinder {
    NonFutureVoidBinder(T value, F continuation, std::shared_ptr<PromiseImpl<U>> promise)
        : value_{ std::move(value) }
        , continuation_{ std::move(continuation) }
        , promise_{ std::move(promise) }
    {
    }

    NonFutureVoidBinder(const NonFutureVoidBinder<T, F, U>&) = delete;
    NonFutureVoidBinder(NonFutureVoidBinder<T, F, U>&& o) = default;

    void operator()()
    {
        try {
            maybe_unpack_and_call<T>{}(continuation_, std::move(value_));
            promise_->set_value();
        } catch (...) {
            auto e = std::current_exception();
            promise_->set_exception(e);
        }
    }

    T value_;
    static_assert(!std::is_reference<F>::value, "F must not be a reference type!");
    F continuation_;
    std::shared_ptr<PromiseImpl<U>> promise_;
};

template <typename F, typename U> struct NonFutureVoidBinder<void, F, U> {
    NonFutureVoidBinder(F continuation, std::shared_ptr<PromiseImpl<U>> promise)
        : continuation_{ std::move(continuation) }
        , promise_{ std::move(promise) }
    {
    }

    NonFutureVoidBinder(const NonFutureVoidBinder<void, F, U>&) = delete;
    NonFutureVoidBinder(NonFutureVoidBinder<void, F, U>&& o) = default;

    void operator()()
    {
        try {
            continuation_();
        } catch (...) {
            auto e = std::current_exception();
            promise_->set_exception(e);
            return;
        }
        promise_->set_value();
    }

    static_assert(!std::is_reference<F>::value, "F must not be a reference type!");
    F continuation_;
    std::shared_ptr<PromiseImpl<U>> promise_;
};

template <typename T, typename F, typename U> struct NonFutureValueBinder {
    NonFutureValueBinder(T value, F continuation, std::shared_ptr<PromiseImpl<U>> promise)
        : value_{ std::move(value) }
        , continuation_{ std::move(continuation) }
        , promise_{ std::move(promise) }
    {
    }

    NonFutureValueBinder(const NonFutureValueBinder<T, F, U>&) = delete;
    NonFutureValueBinder(NonFutureValueBinder<T, F, U>&& o) = default;

    void operator()()
    {
        try {
            auto result = maybe_unpack_and_call<T>{}(continuation_, std::move(value_));
            promise_->set_value(std::forward<U>(result));
        } catch (...) {
            auto e = std::current_exception();
            promise_->set_exception(e);
        }
    }

    T value_;
    static_assert(!std::is_reference<F>::value, "F must not be a reference type!");
    F continuation_;
    std::shared_ptr<PromiseImpl<U>> promise_;
};

template <typename F, typename U> struct NonFutureValueBinder<void, F, U> {
    NonFutureValueBinder(F continuation, std::shared_ptr<PromiseImpl<U>> promise)
        : continuation_{ std::move(continuation) }
        , promise_{ std::move(promise) }
    {
    }

    NonFutureValueBinder(const NonFutureValueBinder<void, F, U>&) = delete;
    NonFutureValueBinder(NonFutureValueBinder<void, F, U>&& o) = default;

    void operator()()
    {
        try {
            auto result = continuation_();
            promise_->set_value(std::forward<U>(result));
        } catch (...) {
            auto e = std::current_exception();
            promise_->set_exception(e);
        }
    }

    static_assert(!std::is_reference<F>::value, "F must not be a reference type!");
    F continuation_;
    std::shared_ptr<PromiseImpl<U>> promise_;
};

/// select which binder to dispatch to for each combination of
///
/// * future or value returning continuations
/// * which continue value or void futures
template <typename T, typename F, typename C>
using select_binder = boost::mp11::mp_if<
    typename future_traits<typename continuation_result_type<F, T>::type>::is_future,
    boost::mp11::mp_if<
        typename std::is_void<C>::type,
        FutureVoidBinder<T, F, C>,
        FutureValueBinder<T, F, C>>,
    boost::mp11::mp_if<
        typename std::is_void<C>::type,
        NonFutureVoidBinder<T, F, C>,
        NonFutureValueBinder<T, F, C>>>;

template <typename T, typename C> struct make_continuation {
    template <typename F>
    static fu2::unique_function<void(T)> create(
        std::shared_ptr<IExecutor> executor,
        F&& continuation,
        const std::shared_ptr<PromiseImpl<continuation_future_element_type<T, F>>>& promise)
    {
        return [executor{ std::move(executor) },
                continuation{ std::forward<F>(continuation) },
                promise](T value) mutable {
            executor->post(select_binder<T, typename std::remove_reference<F>::type, C>{
                std::move(value), std::move(continuation), std::move(promise) });
        };
    }
};

template <typename C> struct make_continuation<void, C> {
    template <typename F>
    static fu2::unique_function<void()> create(
        std::shared_ptr<IExecutor> executor,
        F&& continuation,
        const std::shared_ptr<PromiseImpl<C>>& promise)
    {
        return [executor{ std::move(executor) },
                continuation{ std::forward<F>(continuation) },
                promise]() mutable {
            executor->post(select_binder<void, typename std::remove_reference<F>::type, C>{
                std::move(continuation), std::move(promise) });
        };
    }
};

} // namespace

template <typename T>
FutureImplBase<T>::FutureImplBase()
    : state_{ future_state::Ready<T>{} }
    , continuationSet_(false)
    , onErrorSet_(false)
{
}

template <typename T>
template <typename F>
std::shared_ptr<FutureImpl<continuation_future_element_type<T, F>>>
FutureImplBase<T>::then(F&& continuation)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (continuationSet_) {
        throw std::runtime_error("only one continuation may be scheduled on a future");
    }
    continuationSet_ = true;

    using ContinuationT = continuation_future_element_type<T, F>;

    std::shared_ptr<FutureImpl<ContinuationT>> future;
    std::shared_ptr<PromiseImpl<ContinuationT>> promise;
    std::tie(future, promise) = make_lazy_future_impl<ContinuationT>();

    auto continuationTmp = make_continuation<T, ContinuationT>::create(
        this_thread::get_current_executor(), std::forward<F>(continuation), std::move(promise));

    std::visit(
        boost::hana::overload(
            [&future, &continuationTmp](future_state::Ready<T>& ready) {
                ready.errorObserver_ = future;
                ready.continuation_ = std::move(continuationTmp);
            },
            [this, &continuationTmp](future_state::Resolved<T>& resolved) {
                resolved.callContinuation(continuationTmp);
                state_ = future_state::Continued{};
            },
            [&future](future_state::Rejected& rejected) {
                future->notify_error_ready(rejected.error_);
            },
            [](future_state::Continued&) {
                // Registering a continuation on a future where the error continuation was already
                // called is useless because it would never be triggered.
            }),
        state_);

    return future;
}

template <typename T> template <typename F> void FutureImplBase<T>::catch_error(F&& errorCallback)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (onErrorSet_) {
        throw std::runtime_error("only one error continuation may be scheduled on a future");
    }
    onErrorSet_ = true;
    errorBreaksContinuationChain_ = true;

    using R = typename std::invoke_result<F, std::exception_ptr>::type;
    static_assert(std::is_same<R, void>::value, "Error continuations can not return values!");

    std::visit(
        boost::hana::overload(
            [&errorCallback](future_state::Ready<T>& ready) {
                ready.onError_ = [executor{ this_thread::get_current_executor() },
                                  errorCallback{ std::forward<F>(errorCallback) }](
                                     std::exception_ptr error) mutable {
                    executor->post([error, errorCallback{ std::move(errorCallback) }]() mutable {
                        errorCallback(error);
                    });
                };
            },
            [](future_state::Resolved<T>&) {
                // Registering an error continuation on a resolved future is useless
                // because it would never be triggered.
            },
            [this, &errorCallback](future_state::Rejected& rejected) {
                this_thread::get_current_executor()->post(
                    [error{ rejected.error_ },
                     errorCallback{ std::forward<F>(errorCallback) }]() mutable {
                        errorCallback(error);
                    });
                state_ = future_state::Continued{};
            },
            [](future_state::Continued&) {
                // Registering an error continuation on a continued future is useless
                // because it would never be triggered.
            }),
        state_);
}

template <typename T>
template <typename F>
void FutureImplBase<T>::catch_and_forward_error(F&& errorCallback)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (onErrorSet_) {
        throw std::runtime_error("only one error continuation may be scheduled on a future");
    }
    onErrorSet_ = true;
    errorBreaksContinuationChain_ = false;

    using R = typename std::invoke_result<F, std::exception_ptr>::type;
    static_assert(std::is_same<R, void>::value, "Error continuations can not return values!");

    std::visit(
        boost::hana::overload(
            [&errorCallback](future_state::Ready<T>& ready) {
                ready.onError_ = [executor{ this_thread::get_current_executor() },
                                  errorCallback{ std::forward<F>(errorCallback) }](
                                     std::exception_ptr error) mutable {
                    executor->post([error, errorCallback{ std::move(errorCallback) }]() mutable {
                        errorCallback(error);
                    });
                };
            },
            [](future_state::Resolved<T>&) {
                // Registering an error continuation on a resolved future is useless
                // because it would never be triggered.
            },
            [&errorCallback](future_state::Rejected& rejected) {
                this_thread::get_current_executor()->post(
                    [error{ rejected.error_ },
                     errorCallback{ std::forward<F>(errorCallback) }]() mutable {
                        errorCallback(error);
                    });
            },
            [](future_state::Continued&) {
                // Registering an error continuation on a continued future is useless
                // because it would never be triggered.
            }),
        state_);
}

template <typename T> void FutureImplBase<T>::notify_error_ready(std::exception_ptr error)
{
    std::unique_lock<std::mutex> lock(FutureImplBase<T>::mutex_);

    auto ready = std::get_if<future_state::Ready<T>>(&state_);
    if (!ready) {
        throw std::runtime_error("future already in final state");
    }

    if (ready->onError_) {
        try {
            ready->onError_(error);
        } catch (const ExecutorStoppedException&) {
        }
        if (!errorBreaksContinuationChain_) {
            if (auto errorObserver = ready->errorObserver_.lock()) {
                errorObserver->notify_error_ready(error);
            }
        }
        state_ = future_state::Continued{};
    } else {
        if (auto errorObserver = ready->errorObserver_.lock()) {
            errorObserver->notify_error_ready(error);
        }
        state_ = future_state::Rejected{ error };
    }
}

template <typename T> void FutureImpl<T>::notify_value_ready(const T& value)
{
    std::unique_lock<std::mutex> lock(this->mutex_);

    auto ready = std::get_if<future_state::Ready<T>>(&this->state_);
    if (!ready) {
        throw std::runtime_error("future already in final state");
    }

    if (ready->continuation_) {
        try {
            ready->continuation_(value);
        } catch (const ExecutorStoppedException&) {
        }
        this->state_ = future_state::Continued{};
    } else {
        this->state_ = future_state::Resolved<T>{ value };
    }
}

template <typename T> void FutureImpl<T>::notify_value_ready(T&& value)
{
    std::unique_lock<std::mutex> lock(this->mutex_);

    auto ready = std::get_if<future_state::Ready<T>>(&this->state_);
    if (!ready) {
        throw std::runtime_error("future already in final state");
    }

    if (ready->continuation_) {
        try {
            ready->continuation_(std::forward<T>(value));
        } catch (const ExecutorStoppedException&) {
        }
        this->state_ = future_state::Continued{};
    } else {
        this->state_ = future_state::Resolved<T>{ std::forward<T>(value) };
    }
}

inline void FutureImpl<void>::notify_value_ready()
{
    std::unique_lock<std::mutex> lock(mutex_);

    auto ready = std::get_if<future_state::Ready<void>>(&state_);
    if (!ready) {
        throw std::runtime_error("future already in final state");
    }

    if (ready->continuation_) {
        try {
            ready->continuation_();
        } catch (const ExecutorStoppedException&) {
        }
        state_ = future_state::Continued{};
    } else {
        state_ = future_state::Resolved<void>{};
    }
}

template <typename T>
void future_state::Resolved<T>::callContinuation(resolve_handler_t<T>& continuation)
{
    continuation(std::forward<T>(value_));
}

inline void future_state::Resolved<void>::callContinuation(resolve_handler_t<void>& continuation)
{
    continuation();
}
} // namespace asyncly::detail
