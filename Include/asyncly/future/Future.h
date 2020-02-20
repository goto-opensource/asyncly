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

#include <thread>
#include <type_traits>

// MSVC erroneusly gives warning C4505: unreferenced local function
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4505)
#endif

#include "asyncly/future/detail/Coroutine.h"
#include "asyncly/future/detail/Future.h"
#include "asyncly/future/detail/FutureImpl.h"

///
/// \file
/// \brief Future Library
///
/// TODO(juppm): write a small introduction to the library
///
/// Example usage:
/// \snippet FutureTest.cpp Future Chain
///
namespace asyncly {

template <typename T> class Future;
template <typename T> class Promise;

///
/// make_ready_future creates a resolved `Future<void>`.
///
inline Future<void> make_ready_future();

///
/// make_ready_future<T> creates a future from a value.
///
/// Example usage:
/// \snippet FutureTest.cpp Future Chain
///
/// \tparam T the type of the `Future` to be created resolves to, usually deduced
///
/// \param value the value the newly created Future should resolve to immediately
///
template <typename T> Future<typename std::decay<T>::type> make_ready_future(T value);

///
/// make_exceptional_future creates a future that immediately fails
///
/// Example usage:
/// \snippet FutureTest.cpp Make Exceptional Future
///
/// \tparam T the type of the `Future` to be created. The future will
/// never resolve to any value of this type.
/// \tparam E the type of the error, should be deducted
///             * std::exception_ptr propagates the exception contained within
///             * a base class of std::exception
///             * std::string or char* will be converted to an
///               std::runtime_error with `e` as reason
/// \param e the error to propagate
///
template <typename T, typename E> Future<T> make_exceptional_future(E e);

///
/// make_lazy_future returns a `Future` that can be resolved at a
/// later point in time, together with a `Promise` object that can be
/// used to transfer the value to the `Future`. Usually the `Future`
/// is returned to the caller, while the `Promise` is kept by the
/// callee and resolved at a later point in time. `make_lazy_future`
/// should mainly be used for interacting with third-party or legacy
/// code that does not return futures by itself. It is more desirable
/// for most code using this library to not return lazy futures, but
/// to return `Futures` obtained by transforming other `Futures` using
/// `Future::then`, `when_all`, or `when_any`.
///
/// Example usage:
/// \snippet FutureTest.cpp Lazy Future
///
/// \tparam T the type of the `Future` to be created resolves to
///
/// \return an `std::tuple` containing a `Future<T>` and its
/// corresponding `Promise<T>`
///
template <typename T> std::tuple<Future<T>, Promise<T>> make_lazy_future();

namespace detail {
template <typename T> std::shared_ptr<detail::FutureImpl<T>> get_future_impl(Future<T>& future);
template <typename T>
Future<T> make_future_from_impl(const std::shared_ptr<detail::FutureImpl<T>>& futureImpl);
#ifdef ASYNCLY_HAS_COROUTINES
template <typename T> struct coro_awaiter;
#endif
}

///
/// Future is a container for a value that will potentially available at a later point in time.
///
template <typename T> class Future {
  public:
    using element_type = T;
    using value_type = T;

#ifdef ASYNCLY_HAS_COROUTINES
    using promise_type = detail::coro_promise<T>;
    detail::coro_awaiter<T> operator co_await()
    {
        return detail::coro_awaiter<T>(this);
    }
    friend struct detail::coro_awaiter<T>;
#endif
    friend std::tuple<Future<T>, Promise<T>> make_lazy_future<T>();
    friend Future<T> make_exceptional_future<T, std::exception_ptr>(std::exception_ptr e);

    friend Future<T> make_ready_future<T>(T value);
    friend std::shared_ptr<detail::FutureImpl<T>> detail::get_future_impl<T>(Future<T>&);

  public:
    Future(const Future<T>&) = default;
    Future<T>& operator=(const Future<T>&) = default;

  private:
    const std::shared_ptr<detail::FutureImpl<T>> futureImpl_;

  public:
    ///
    /// `then` allows access and transformation of the value contained by
    /// `Future` once it's been resolved. The supplied continuation will
    /// be scheduled on the current executor (the executor the current
    /// task has been dispatched to). Calling `then` from outside of
    /// an executor task will throw an exception.
    ///
    /// Example usage:
    /// \snippet FutureTest.cpp Future Chain
    ///
    /// \tparam F a functor type, must always be deduced
    ///
    /// \param f a continuation that will be called once the `Future`
    /// is resolved. The continuation gets passed the value the
    /// `Future` resolves to. There are three different categories of
    /// return values that are supported by continuations and
    /// determine the return type of `then`:
    ///
    ///   * `void`, which will cause `then` to return a `Future<void>`
    ///    that is resolved once the continuation has been run
    ///
    ///   * `Future<S>`, with `S` possibly different from `T` will
    ///     cause `then` to return a `Future<S>`, which will be
    ///     resolved with a value of `S` once the `Future<S>` returned
    ///     by `f` is resolved
    ///
    ///   * a non-future value `u` of type `U`, which will cause
    ///     `then` to return a `Future<U>`, which will be resolved
    ///     with the value returned by `f`
    ///
    /// \throw throws an std::runtime_error when called more than once
    ///
    template <typename F> auto then(F&& f) -> Future<detail::continuation_future_element_type<T, F>>
    {
        return detail::make_future_from_impl(futureImpl_->then(std::forward<F>(f)));
    }

    ///
    /// `catch_error` allows setting an error handler on a
    /// `Future`. If a `Future` is rejected, the handler is
    /// called with a `std::exception_ptr` parameter. There are
    /// multiple situations in which this can occur:
    ///
    ///   * the callee decides that an error occurred that prevents
    ///     the `Future` from being resolved
    ///
    ///   * an exception occurred in a `then` continuation
    ///
    ///   * a `Future` earlier in a `then` transformation chain cannot
    ///     be resolved, causing the input to following continuations
    ///     to not be available
    ///
    /// Inserting a `Future` produced by `catch_error` into a
    /// continuation chain will propagate non-error values further
    /// down the chain while aborting the whole chain after the
    /// `catch_error`. There can be multiple `catch_error`s in a chain
    /// to handle errors occurring at different points in the chain.
    ///
    /// Example usage:
    /// \snippet FutureTest.cpp Error Chain
    ///
    /// \throw throws an std::runtime_error when there is already an error handler set
    ///
    template <typename F> Future<T>& catch_error(F&& f)
    {
        futureImpl_->catch_error(std::forward<F>(f));
        return *this;
    }

    ///
    /// `catch_and_forward_error` allows setting an error handler on a
    /// `Future`. If a `Future` is rejected, the handler is
    /// called with a `std::exception_ptr` parameter. There are
    /// multiple situations in which this can occur:
    ///
    ///   * the callee decides that an error occurred that prevents
    ///     the `Future` from being resolved
    ///
    ///   * an exception occurred in a `then` continuation
    ///
    ///   * a `Future` earlier in a `then` transformation chain cannot
    ///     be resolved, causing the input to following continuations
    ///     to not be available
    ///
    /// In contrast to `catch_error` the error handler will be called AND
    /// the error will be propagated further down the chain after the
    /// `catch_and_forward_error`. There can be multiple `catch_error`s in
    /// a chain to handle errors occurring at different points in the chain.
    ///
    /// Example usage:
    /// \snippet FutureTest.cpp Error Chain
    ///
    /// \throw throws an std::runtime_error when there is already an error handler set
    ///
    template <typename F> Future<T>& catch_and_forward_error(F&& f)
    {
        futureImpl_->catch_and_forward_error(std::forward<F>(f));
        return *this;
    }

  public:
    Future(const std::shared_ptr<detail::FutureImpl<T>>& futureImpl)
        : futureImpl_{ futureImpl }
    {
    }
};

template <> class Future<void> {
  public:
    using element_type = void;
    using value_type = void;
#ifdef ASYNCLY_HAS_COROUTINES
    using promise_type = detail::coro_promise<void>;
    detail::coro_awaiter<void> operator co_await()
    {
        return detail::coro_awaiter<void>(this);
    }
    friend struct detail::coro_awaiter<void>;
#endif

    friend std::tuple<Future<void>, Promise<void>> make_lazy_future();
    friend Future<void> make_exceptional_future<void, std::exception_ptr>(std::exception_ptr e);

    friend Future<void> make_ready_future();
    friend std::shared_ptr<detail::FutureImpl<void>> detail::get_future_impl<void>(Future<void>&);
    friend Future<void>
    detail::make_future_from_impl<void>(const std::shared_ptr<detail::FutureImpl<void>>&);

  public:
    Future(const Future<void>&) = default;

  private:
    const std::shared_ptr<detail::FutureImpl<void>> futureImpl_;

  public:
    template <typename F>
    auto then(F&& f) -> Future<detail::continuation_future_element_type<void, F>>
    {
        return detail::make_future_from_impl(futureImpl_->then(std::forward<F>(f)));
    }

    template <typename F> Future<void>& catch_error(F&& f)
    {
        futureImpl_->catch_error(std::forward<F>(f));
        return *this;
    }

    template <typename F> Future<void>& catch_and_forward_error(F&& f)
    {
        futureImpl_->catch_and_forward_error(std::forward<F>(f));
        return *this;
    }

  public:
    Future(const std::shared_ptr<detail::FutureImpl<void>>& futureImpl)
        : futureImpl_{ futureImpl }
    {
    }
};

// workaround for VS2013 SFINAE bugs
namespace detail {
template <typename T, typename E> struct SetException {
    static_assert(
        std::is_base_of<std::exception, E>::value,
        "your error types should inherit from std::exception");
    static void dispatch(T& promise, E e)
    {
        SetException<T, std::exception_ptr>::dispatch(promise, std::make_exception_ptr(e));
    }
};

template <typename T> struct SetException<T, std::exception_ptr> {
    static void dispatch(T& promise, std::exception_ptr e)
    {
        promise.promiseImpl_->set_exception(e);
    }
};

template <typename T> struct SetException<T, std::string> {
    static void dispatch(T& promise, std::string e)
    {
        SetException<T, std::runtime_error>::dispatch(promise, std::runtime_error{ e });
    }
};

template <typename T> struct SetException<T, char const*> {
    static void dispatch(T& promise, char const* e)
    {
        SetException<T, std::runtime_error>::dispatch(promise, std::runtime_error{ e });
    }
};
}
template <typename T> class Promise {
  public:
    friend std::tuple<Future<T>, Promise<T>> make_lazy_future<T>();
    friend detail::SetException<Promise<T>, std::exception_ptr>;

    void set_value(T&& value)
    {
        promiseImpl_->set_value(std::move(value));
    }

    void set_value(const T& value)
    {
        promiseImpl_->set_value(value);
    }

    template <typename E> void set_exception(E e)
    {
        detail::SetException<Promise<T>, E>::dispatch(*this, e);
    }

    Future<T> get_future()
    {
        return { promiseImpl_->get_future() };
    }

    Promise()
        : promiseImpl_{ std::get<1>(detail::make_lazy_future_impl<T>()) }
    {
    }

  private:
    Promise(const std::shared_ptr<detail::PromiseImpl<T>>& promiseImpl)
        : promiseImpl_(promiseImpl)
    {
    }

    const std::shared_ptr<detail::PromiseImpl<T>> promiseImpl_;
};

template <> class Promise<void> {
  public:
    friend std::tuple<Future<void>, Promise<void>> make_lazy_future<void>();
    friend detail::SetException<Promise<void>, std::exception_ptr>;

    void set_value()
    {
        promiseImpl_->set_value();
    }

    template <typename E> void set_exception(E e)
    {
        detail::SetException<Promise<void>, E>::dispatch(*this, e);
    }

    Future<void> get_future()
    {
        return { promiseImpl_->get_future() };
    }

    Promise()
        : promiseImpl_{ std::get<1>(detail::make_lazy_future_impl<void>()) }
    {
    }

  private:
    Promise(const std::shared_ptr<detail::PromiseImpl<void>>& promiseImpl)
        : promiseImpl_(promiseImpl)
    {
    }

    const std::shared_ptr<detail::PromiseImpl<void>> promiseImpl_;
};

inline Future<void> make_ready_future()
{
    return { detail::make_ready_future_impl() };
}

template <typename T> Future<typename std::decay<T>::type> make_ready_future(T value)
{
    return { detail::make_ready_future_impl<typename std::decay<T>::type>(std::move(value)) };
}

namespace {
// workaround for SFINAE bugs in VS2013

template <typename T, typename E> struct MakeExceptionalFuture {
    static_assert(
        std::is_base_of<std::exception, E>::value,
        "your error types should inherit from std::exception");
    static Future<T> dispatch(E e)
    {
        return MakeExceptionalFuture<T, std::exception_ptr>::dispatch(std::make_exception_ptr(e));
    }
};

template <typename T> struct MakeExceptionalFuture<T, std::exception_ptr> {
    static Future<T> dispatch(std::exception_ptr e)
    {
        return { detail::make_exceptional_future_impl<T>(e) };
    }
};

template <typename T> struct MakeExceptionalFuture<T, std::string> {
    static Future<T> dispatch(std::string e)
    {
        return MakeExceptionalFuture<T, std::runtime_error>::dispatch(std::runtime_error{ e });
    }
};

template <typename T> struct MakeExceptionalFuture<T, char const*> {
    static Future<T> dispatch(char const* e)
    {
        return MakeExceptionalFuture<T, std::runtime_error>::dispatch(std::runtime_error{ e });
    }
};
}

template <typename T, typename E> Future<T> make_exceptional_future(E e)
{
    return MakeExceptionalFuture<T, E>::dispatch(e);
}

template <typename T> std::tuple<Future<T>, Promise<T>> make_lazy_future()
{
    std::shared_ptr<detail::FutureImpl<T>> futureImpl;
    std::shared_ptr<detail::PromiseImpl<T>> promiseImpl;
    std::tie(futureImpl, promiseImpl) = detail::make_lazy_future_impl<T>();
    return std::make_tuple(
        detail::make_future_from_impl(std::move(futureImpl)), Promise<T>(std::move(promiseImpl)));
}

namespace detail {
template <typename T> std::shared_ptr<detail::FutureImpl<T>> get_future_impl(Future<T>& future)
{
    return future.futureImpl_;
}

template <typename T>
Future<T> make_future_from_impl(const std::shared_ptr<detail::FutureImpl<T>>& futureImpl)
{
    return { futureImpl };
}
}
}

#include "asyncly/future/detail/CoroutineImpl.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif
