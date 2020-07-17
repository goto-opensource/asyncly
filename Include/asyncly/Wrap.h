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
#include <memory>
#include <stdexcept>
#include <utility>

#ifdef _MSC_VER
#pragma warning(push)
// warning C4459: declaration of 'to' hides global declaration
#pragma warning(disable : 4459)
#endif

#include <boost/hana.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "asyncly/executor/CurrentExecutor.h"
#include "asyncly/executor/IExecutor.h"

namespace asyncly {

/// \defgroup wrap functions
///
/// In general wrap functions create new function objects by taking an existing function object as
/// input as well as some additional parameters. They extend the functionality by
/// guarding/surrounding the execution of the given function object with common patterns. The types
/// of wrapping can be categorized into post, weak and weak_post:
///
/// post: Returns a new function object that, when executed, posts the user-provided function object
/// to the specified executor.
/// Post wrappers are mostly used in cases where code that already runs within an executor context
/// (object A) calls code that does not yet use Futures/Observables but requires callbacks (object
/// B). Without a post, the code that triggers the callback from within object B will execute the
/// provided callback synchronously within its own thread context. This breaks the thread safety
/// requirements where all mutating data accesses to objects within an executor context have to be
/// done from within the executor thread. Therefore the callback function object can not directly
/// access data from A. The callback first has to post the mutating calls to A's executor. Using the
/// wrap_post wrapper ensures that the callback is executed from the executor context of object A.
/// NOTE: It is not necessary to use wrap_post for function objects passed to `Future<T>::then`.
/// `Future<T>::then` already synchronously retrieves the current executor in the calling thread and
/// the continuation is automatically posted into this executor when the Future is set.
///
/// +------------------+-------------------+
/// | executor         | function          |
/// +------------------+-------------------+
/// | given executor   | wrap_post         |
/// +------------------+-------------------+
/// | current executor | wrap_post_current |
/// +------------------+-------------------+
///
/// weak: Given a weak_ptr, returns a new function object that, when executed, invokes the
/// user-provided function only if the weak_ptr can be locked. Multiple wrap_weak variants are
/// available to select a behavior when locking the weak_ptr fails. Additional variants are
/// available to construct the weak_ptr from a user-provided raw pointer (often this), which is
/// expected to implement enable_shared_from_this. The locking error behaviour can be specified
/// (lock error).
/// The main usage scenario for wrap_weak is to decouple the lifetime of data/objects that are
/// captured in continuations given to futures (`Future<T>::then`) from the lifetime of the
/// Future/Promise possibly held by a different executor thread. A continuation should in nearly all
/// cases NOT capture data by raw pointer or reference because those objects may not be alive
/// anymore when the Promise is set and the continuation is executed, which may lead to a crash.
/// Ideally, continuations should hold as few state as possible, including references to state
/// outside the continuation. However, this is not always possible. Sometimes you want to call
/// methods of the current object. Therefore you might be tempted to write
/// `[this](){callSomeMember();}` which is in nearly all cases a very BAD idea. When the current
/// object gets deleted before the continuation is executed, the call will most probably lead to a
/// crash.
/// The safest way to protect such continuations is to additionally capture a weak_ptr
/// pointing to the current object. When the function object is called, the weak_ptr is lock()-ed
/// and only if this was successful (the object was still alive when the continuation was executed),
/// the rest of the function object is executed. Furthermore, the shared_ptr returned by the lock()
/// call must be held alive until the continuation finished. This is necessary to keep the object
/// alive because the shared_ptr might get the last strong reference to the object. `wrap_weak` adds
/// exactly this code so that it does not have to be reimplemented in every continuation.
/// The `_ignore` versions are useful for cases where you do not want to make the callee aware of
/// the locking error. This is the case when:
/// * the function object is directly posted into an executor
///   `executor->post(wrap_weak_ignore(...))`
///   because an exception due to the failed lock would make the executor stop
/// * or the function object is used as `Future` error handler
///   `.catch_error(wrap_weak_ignore(...))`
///   which would also stop the corresponding executor due to the exception that is thrown
/// The `_ignore` versions should NOT be used in `Future.then(wrap_weak(...))` success handlers.
/// For non-void value-returning continuations the `_ignore` versions do not compile. However, for
/// void `Futures` ignoring the error would be treated as successfully resolved `Future`
/// continuation, which is not the desired effect. When we do not use the `_ignore` versions and a
/// lock error occures, the thrown exception will correctly be handled and the continuation `Future`
/// will be rejected.
///
/// +----------+--------------------+----------------------------------+
/// | object   | lock error         | function                         |
/// +----------+--------------------+----------------------------------+
/// | weak_ptr | std::runtime_error | wrap_weak                        |
/// +          +--------------------+----------------------------------+
/// |          | custom error       | wrap_weak_with_custom_error      |
/// +          +--------------------+----------------------------------+
/// |          | ignore             | wrap_weak_ignore                 |
/// +----------+--------------------+----------------------------------+
/// | this     | std::runtime_error | wrap_weak_this                   |
/// +          +--------------------+----------------------------------+
/// |          | custom error       | wrap_weak_this_with_custom_error |
/// +          +--------------------+----------------------------------+
/// |          | ignore             | wrap_weak_this_ignore            |
/// +----------+--------------------+----------------------------------+
///
/// weak_post: A combination of the versions above, which return a new function object that, when
/// executed, posts the user-provided function to the specified executor and invokes the function
/// only if the user-provided weak_ptr can be locked at the time of execution.
///
/// +------------------+----------+--------------------+------------------------------------+
/// | executor         | object   | lock error         | function                           |
/// +------------------+----------+--------------------+------------------------------------+
/// | given executor   | weak_ptr | std::runtime_error | n/a                                |
/// +                  +          +--------------------+------------------------------------+
/// |                  |          | custom error       | wrap_weak_post_with_custom_error   |
/// +                  +          +--------------------+------------------------------------+
/// |                  |          | ignore             | wrap_weak_post_ignore              |
/// +                  +----------+--------------------+------------------------------------+
/// |                  | this     | std::runtime_error | n/a                                |
/// +                  +          +--------------------+------------------------------------+
/// |                  |          | custom error       | n/a                                |
/// +                  +          +--------------------+------------------------------------+
/// |                  |          | ignore             | wrap_weak_this_post_ignore         |
/// +------------------+----------+--------------------+------------------------------------+
/// | current executor | weak_ptr | std::runtime_error | n/a                                |
/// +                  +          +--------------------+------------------------------------+
/// |                  |          | custom error       | n/a                                |
/// +                  +          +--------------------+------------------------------------+
/// |                  |          | ignore             | wrap_weak_post_current_ignore      |
/// +                  +----------+--------------------+------------------------------------+
/// |                  | this     | std::runtime_error | n/a                                |
/// +                  +          +--------------------+------------------------------------+
/// |                  |          | custom error       | n/a                                |
/// +                  +          +--------------------+------------------------------------+
/// |                  |          | ignore             | wrap_weak_this_post_current_ignore |
/// +------------------+----------+--------------------+------------------------------------+
///
///@{
/// wrap_post returns a function object of type F that, when called, posts a given function object
/// to an executor. All arguments that F takes are copied into the task dispatched to the executor.
/// Copying is necessary to avoid data races between threads.
///
/// Example usage:
///     // createConnection gets a lambda that notifies when a connection has been created.
///     // We want this to happen on our thread so we don't get data races.
///     connection->createConnection(asyncly::wrap_post(executor_, [this](auto connection) {
///     this->connection_ = connection; });
///
template <typename F> auto wrap_post(const asyncly::IExecutorPtr& executor, F function)
{
    return [executor, t = std::move(function)](auto&&... args) mutable {
        executor->post([cargs = boost::hana::make_tuple(std::forward<decltype(args)>(args)...),
                        f = std::move(t)]() mutable {
            boost::hana::unpack(cargs, [cf = std::move(f)](auto&&... fargs) mutable {
                cf(std::forward<decltype(fargs)>(fargs)...);
            });
        });
    };
}

/// wrap_post_current works similar to wrap_post, but gets the executor executing the current task
/// as a convenience.
template <typename F> auto wrap_post_current(F function)
{
    return wrap_post(this_thread::get_current_executor(), std::move(function));
}

/// wrap_weak_with_custom_error works like wrap_weak, but lets you specify custom behavior for the
/// case in which weak pointers have expired.
template <typename T, typename F, typename E> // T models std::weak_ptr or std::shared_ptr
auto wrap_weak_with_custom_error(const T& object, F function, E errorFunction)
{
    using U = typename T::element_type;
    return [weakObject = std::weak_ptr<U>{ object },
            cfunction = std::move(function),
            efunction = std::move(errorFunction)](auto&&... args) mutable {
        if (const auto locked = weakObject.lock()) {
            return cfunction(locked, std::forward<decltype(args)>(args)...);
        } else {
            return efunction();
        }
    };
}

/// wrap_weak returns a function object wrapping a weak pointer and a function together
/// so that the function is only executed when the weak pointer isn't expired. The function is
/// passed the locked weak pointer as its first argument in this case. Other arguments are
/// forwarded. This is helpful when using shared ownership in conjunction with Future continuations
/// (see example). In case the weak pointer is expired, the produced function object throws an
/// std::runtime_error. You can customize this behavior by using wrap_weak_with_custom_error instead
/// of this function.
///
///     connection->createConnection()
///           .then(
///                  asyncly::wrap_weak(
///                           shared_from_this(),
///                           [](auto self, auto connection) {
///                                 self->connection_ = connection;
///                           }));
///
template <typename T, typename F> // T models std::weak_ptr or std::shared_ptr
auto wrap_weak(const T& object, F function)
{
    // ideally, we'd want to reuse wrap_weak_with_custom_error, but type inference in C++14
    // won't let us. This might be fixed in C++20. The reason is that the return type of the
    // error function cannot be inferred when it only throws. It also cannot be computed
    // when users use auto in `function`.
    // return wrap_weak_with_custom_error(
    //     object, std::move(function), []() -> boost::callable_traits::return_type_t<F> {
    //         throw std::runtime_error("weak wrapped object expired");
    //     });

    using U = typename T::element_type;
    return [weakObject = std::weak_ptr<U>{ object },
            cfunction = std::move(function)](auto&&... args) mutable {
        if (const auto locked = weakObject.lock()) {
            return cfunction(locked, std::forward<decltype(args)>(args)...);
        } else {
            throw std::runtime_error("weak wrapped object expired");
        }
    };
}

/// wrap_weak_ignore works like wrap_weak, but ignores expired weak pointers instead of throwing
template <typename T, typename F> // T models std::weak_ptr or std::shared_ptr
auto wrap_weak_ignore(const T& object, F function)
{
    return wrap_weak_with_custom_error(object, std::move(function), []() {});
}

/// wrap_weak_post_ignore returns a function object of type F that executes a task based on whether
/// the supplied weak pointer is still alive. When the supplied weak pointer is no longer alive, the
/// task exits without doing anything, otherwise the weak pointer is locked and the corresponding
/// shared_ptr is passed to the supplied function. All further arguments that F takes (beyond the
/// shared_ptr corresponding to the `object` parameter to `wrap_weak_post_ignore`) are copied into
/// the task dispatched to the executor. Copying is necessary to avoid data races between threads.
/// Should the weak pointer be expired, `function` is not executed and everything returns. If you
/// want to specify different behavior for this case, `wrap_weak_post_with_custom_error` allows for
/// customization.
///
/// Example usage:
///     // createConnection gets a lambda that notifies when a connection has been created.
///     // We want this to happen on our thread so we don't get data races. Note the difference
///     // in not capturing this compared to the documentation of the `wrap` function above.
///     connection->createConnection(asyncly::wrap_weak_post_ignore(
///                 executor_,
///                 shared_from_this(),
///                 [](auto self, auto connection) { self->connection_ = connection; });
///
template <typename T, typename F> // T models std::weak_ptr or std::shared_ptr
auto wrap_weak_post_ignore(const asyncly::IExecutorPtr& executor, const T& object, F function)
{
    return wrap_weak_post_with_custom_error(executor, object, std::move(function), []() {});
}

/// wrap_weak_post_current_ignore works similar to wrap_weak_post_ignore, but gets the executor
/// executing the current task as a convenience.
template <typename T, typename F> auto wrap_weak_post_current_ignore(const T& object, F function)
{
    return wrap_weak_post_with_custom_error(
        this_thread::get_current_executor(), object, std::move(function), []() {});
}

/// wrap_weak_this_post_ignore works similar to wrap_weak_post_ignore, but you can pass `this` to it
/// and it calls `this->shared_from_this` for you.
template <typename T, typename F> // T models *std::shared_from_this<T>
auto wrap_weak_this_post_ignore(const asyncly::IExecutorPtr& executor, T self, F function)
{
    return wrap_weak_post_with_custom_error(
        executor, self->shared_from_this(), std::move(function), []() {});
}

/// wrap_weak_this_post_current_ignore works similar to wrap_weak_post_ignore, but gets the executor
/// executing the current task as a convenience and you can pass `this` to it and it calls
/// `this->shared_from_this` for you.
template <typename T, typename F> // T models *std::shared_from_this<T>
auto wrap_weak_this_post_current_ignore(T self, F function)
{
    return wrap_weak_post_current_ignore(self->shared_from_this(), std::move(function));
}

/// wrap_weak_post_with_custom_error works like wrap_weak_post_ignore, but allows you to specify
/// what happens should the weak pointer have expired.
template <typename T, typename F, typename E> // T models std::weak_ptr or std::shared_ptr
auto wrap_weak_post_with_custom_error(
    const asyncly::IExecutorPtr& executor, const T& object, F function, E errorFunction)
{
    using U = typename T::element_type;
    return [executor,
            weakObject = std::weak_ptr<U>{ object },
            t = std::move(function),
            cerrorFunction = std::move(errorFunction)](auto&&... args) mutable {
        try {
            executor->post([cargs = boost::hana::make_tuple(std::forward<decltype(args)>(args)...),
                            cweakObject = weakObject,
                            f = std::move(t),
                            ccerrorFunction = std::move(cerrorFunction)]() mutable {
                boost::hana::unpack(
                    cargs,
                    [cf = std::move(f), ce = std::move(ccerrorFunction), &cweakObject](
                        auto&&... fargs) mutable {
                        if (const auto locked = cweakObject.lock()) {
                            cf(locked, std::forward<decltype(fargs)>(fargs)...);
                        } else {
                            ce();
                        }
                    });
            });
        } catch (...) {
            cerrorFunction();
        }
    };
}

/// wrap_weak_this is a convenience function for objects who use std::shared_from_this. It works
/// similar to wrap_weak, but you can pass `this` to it and it calls `this->shared_from_this` for
/// you. In case the pointer is expired when the function throws, an std::runtime_error is thrown.
template <typename T, typename F> // T models *std::shared_from_this<T>
auto wrap_weak_this(T self, F function)
{
    return wrap_weak(self->shared_from_this(), std::move(function));
}

/// wrap_weak_this_ignore works like wrap_weak_this, but ignores expired weak_ptrs instead of
/// throwing. As no value is provided, this only works if F returns void.
template <typename T, typename F> // T models *std::shared_from_this<T>
auto wrap_weak_this_ignore(T self, F function)
{
    return wrap_weak_with_custom_error(self->shared_from_this(), std::move(function), []() {});
}

/// wrap_weak_this_with_custom_error works like wrap_weak_this but allows to specify
/// the error handling callback. As no value is provided, this only works if F returns void.
template <typename T, typename F, typename E> // T models *std::shared_from_this<T>
auto wrap_weak_this_with_custom_error(T self, F function, E errorFunction)
{
    return wrap_weak_with_custom_error(
        self->shared_from_this(), std::move(function), errorFunction);
}
///@}
}
