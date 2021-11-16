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

#include <mutex>
#include <unordered_map>

#include <boost/callable_traits/return_type.hpp>

#include <function2/function2.hpp>

#include "asyncly/observable/Subscription.h"
#include "asyncly/observable/detail/ObservableImpl.h"

namespace asyncly {

template <typename T> class Observable;

/// make_lazy_observable creates an observable with customized behavior of what happens
/// whenever a new subscriber is added to it. Whenever somebody subscribes to the observable,
/// the `onSubscribe` function passed as a parameter is called and is passed a `Subscriber<T>`
/// object that can be used to push values, errors, etc to the subscriber.
/// The current executor is captured and calls to onSubscribe go through it, similar to
/// Future::then(). This means that calls to this function can only be made from inside of an
/// executor task. If calls to this function are made from outside of an executor task (think
/// main()), an exception is thrown.

template <typename T, typename F> Observable<T> make_lazy_observable(F onSubscribe)
{
    auto observable = std::make_shared<detail::ObservableImpl<T>>(
        onSubscribe, asyncly::this_thread::get_current_executor());
    return Observable<T>{ std::move(observable) };
}

/// Observable<T> represents a collection of values of type T distributed in time.
///
///       |  one value  |  collection of values
///-------------------------------------------------
/// space |      T      |          vector<T>
/// time  |  Future<T>  |        Observable<T>
/// ------------------------------------------------
///
template <typename T> class Observable {
  public:
    using value_type = T;

  public:
    /// This should never be called from users of this library,
    /// use factory functions (make_lazy_observable) instead.
    Observable(std::shared_ptr<detail::ObservableImpl<T>> aimpl);

  public:
    /// subscribe registers callbacks to the Observable, that will be called
    /// when the publisher corresponding to the observable pushes values or error/completion events.
    /// Three callbacks can be supplied
    /// * valueFunction receives published values and can potentially be called multiple times
    /// * errorFunction receives an error in form of std::exception_ptr and will be called only once
    /// * completionFunction is called at most once to signal graceful termination
    ///
    /// After receiving a call to errorFunction or completionFunction, no other functions will be
    /// called. nullptr can be supplied for each of the three functions to signal noop functions.
    ///
    /// ## Threading implications
    /// The handler functions passed to subscribe are executed in the task-local executor context.
    /// this means that they can only be called inside of an executor task (similar to
    /// Future::then). It allows business logic code to not deal with threading at all, but it needs
    /// the executor runtime, which is a tradeoff. If calls to this function are made from outside
    /// of an executor task (think main()), an exception is thrown.
    Subscription subscribe(fu2::unique_function<void(T)> valueFunction);
    Subscription subscribe(
        fu2::unique_function<void(T)> valueFunction,
        fu2::unique_function<void(std::exception_ptr e)> errorFunction);
    Subscription subscribe(
        fu2::unique_function<void(T)> valueFunction,
        fu2::unique_function<void(std::exception_ptr e)> errorFunction,
        fu2::unique_function<void()> completionFunction);

    /// map takes a transformation function `f :: U -> T`, similar to std::function<U(T)> that
    /// transforms a value of type T into a value of type U. It returns a new Observable<U>. For
    /// each value of type T that is observed, a new value of type U is obtained by calling `f` and
    /// then emmitted to the Observable<U> returned by this function. Errors and completions are
    /// forwarded as-is. T and U can be the same. See
    /// http://reactivex.io/documentation/operators/map.html.
    template <typename F> // F models std::function<U(T)>
    auto map(F) -> Observable<boost::callable_traits::return_type_t<F>>;

    /// filter takes a filter function F and returns an Observable that emits only those values for
    /// which the application of the filter function to the value returns true (similar to
    /// std::copy_if). See http://reactivex.io/documentation/operators/filter.html.
    template <typename F> // F models std::function<bool(T)>
    Observable<T> filter(F);

    /// scan asynchronously iterates over values in Observable<T> while maintaining state between
    /// applications of the function F over each value. It emits an observable of the current state
    /// after each invocation of F. Think about it like std::accumulate with emiting intermediate
    /// results as an Observable. See http://reactivex.io/documentation/operators/scan.html for a
    /// really good description of the concept. Note that it does not emit the initial state as the
    /// first element, but emits the initial state fed to F together with the first element of the
    /// underlying Observable. The void overload has a slightly different signature for F, because
    /// there is no value to be fed into it, but state only.
    template <typename F> // f models std::function<U(U, T)> with U being any type representing the
                          // state between applications of F to different values of type T
    auto scan(F function, boost::callable_traits::return_type_t<F> initialState)
        -> Observable<boost::callable_traits::return_type_t<F>>;

  private:
    std::shared_ptr<detail::ObservableImpl<T>> impl;
};

template <> class Observable<void> {
  public:
    using value_type = void;

  public:
    Observable(std::shared_ptr<detail::ObservableImpl<void>> aimpl);

  public:
    Subscription subscribe(fu2::unique_function<void()> valueFunction);
    Subscription subscribe(
        fu2::unique_function<void()> valueFunction,
        fu2::unique_function<void(std::exception_ptr e)> errorFunction);
    Subscription subscribe(
        fu2::unique_function<void()> valueFunction,
        fu2::unique_function<void(std::exception_ptr e)> errorFunction,
        fu2::unique_function<void()> completionFunction);

    template <typename F> // F models std::function<U()>
    auto map(F) -> Observable<boost::callable_traits::return_type_t<F>>;

    template <typename F> // F models std::function<U(U)>
    Observable<boost::callable_traits::return_type_t<F>>
        scan(F, boost::callable_traits::return_type_t<F>);

  private:
    std::shared_ptr<detail::ObservableImpl<void>> impl;
};

template <typename T>
Observable<T>::Observable(std::shared_ptr<detail::ObservableImpl<T>> aimpl)
    : impl{ std::move(aimpl) }
{
}

template <typename T>
Subscription Observable<T>::subscribe(fu2::unique_function<void(T)> valueFunction)
{
    return subscribe(std::move(valueFunction), nullptr);
}

template <typename T>
Subscription Observable<T>::subscribe(
    fu2::unique_function<void(T)> valueFunction,
    fu2::unique_function<void(std::exception_ptr e)> errorFunction)
{
    return subscribe(std::move(valueFunction), std::move(errorFunction), nullptr);
}

template <typename T>
Subscription Observable<T>::subscribe(
    fu2::unique_function<void(T)> valueFunction,
    fu2::unique_function<void(std::exception_ptr e)> errorFunction,
    fu2::unique_function<void()> completionFunction)
{
    return impl->subscribe(
        std::move(valueFunction), std::move(errorFunction), std::move(completionFunction));
}

template <typename T>
template <typename F>
auto Observable<T>::map(F function) -> Observable<boost::callable_traits::return_type_t<F>>
{
    using ReturnType = boost::callable_traits::return_type_t<F>;
    return make_lazy_observable<ReturnType>(
        [cimpl = this->impl, f = std::move(function)](auto subscriber) {
            cimpl->subscribe(
                [cf = std::move(f), subscriber](auto value) mutable {
                    subscriber.pushValue(cf(value));
                },
                [subscriber](auto error) mutable { subscriber.pushError(error); },
                [subscriber]() mutable { subscriber.complete(); });
        });
}

template <typename T> template <typename F> Observable<T> Observable<T>::filter(F function)
{
    return make_lazy_observable<T>(
        [cimpl = this->impl, cfunction = std::move(function)](auto subscriber) mutable {
            cimpl->subscribe(
                [ccfunction = std::move(cfunction), subscriber](auto value) mutable {
                    if (ccfunction(value)) {
                        subscriber.pushValue(std::move(value));
                    }
                },
                [subscriber](auto error) mutable { subscriber.pushError(error); },
                [subscriber]() mutable { subscriber.complete(); });
        });
}

template <typename T>
template <typename F>
auto Observable<T>::scan(F function, boost::callable_traits::return_type_t<F> initialState)
    -> Observable<boost::callable_traits::return_type_t<F>>
{
    return make_lazy_observable<boost::callable_traits::return_type_t<F>>(
        [cimpl = this->impl,
         cfunction = std::move(function),
         cinitialState = std::move(initialState)](auto subscriber) mutable {
            cimpl->subscribe(
                [ccfunction = std::move(cfunction), subscriber, state = std::move(cinitialState)](
                    auto value) mutable {
                    state = ccfunction(state, std::move(value));
                    subscriber.pushValue(state);
                },
                [subscriber](auto error) mutable { subscriber.pushError(error); },
                [subscriber]() mutable { subscriber.complete(); });
        });
}

inline Observable<void>::Observable(std::shared_ptr<detail::ObservableImpl<void>> aimpl)
    : impl{ std::move(aimpl) }
{
}

inline Subscription Observable<void>::subscribe(fu2::unique_function<void()> valueFunction)
{
    return subscribe(std::move(valueFunction), nullptr);
}

inline Subscription Observable<void>::subscribe(
    fu2::unique_function<void()> valueFunction,
    fu2::unique_function<void(std::exception_ptr e)> errorFunction)
{
    return subscribe(std::move(valueFunction), std::move(errorFunction), nullptr);
}

inline Subscription Observable<void>::subscribe(
    fu2::unique_function<void()> valueFunction,
    fu2::unique_function<void(std::exception_ptr e)> errorFunction,
    fu2::unique_function<void()> completionFunction)
{
    return impl->subscribe(
        std::move(valueFunction), std::move(errorFunction), std::move(completionFunction));
}

template <typename F>
auto Observable<void>::map(F function) -> Observable<boost::callable_traits::return_type_t<F>>
{
    using ReturnType = boost::callable_traits::return_type_t<F>;
    return make_lazy_observable<ReturnType>(
        [cimpl = this->impl, f = std::move(function)](auto subscriber) {
            cimpl->subscribe(
                [cf = std::move(f), subscriber]() mutable { subscriber.pushValue(cf()); },
                [subscriber](auto error) mutable { subscriber.pushError(error); },
                [subscriber]() mutable { subscriber.complete(); });
        });
}

template <typename F>
auto Observable<void>::scan(F function, boost::callable_traits::return_type_t<F> initialState)
    -> Observable<boost::callable_traits::return_type_t<F>>
{
    return make_lazy_observable<boost::callable_traits::return_type_t<F>>(
        [cimpl = this->impl,
         cfunction = std::move(function),
         cinitialState = std::move(initialState)](auto subscriber) mutable {
            cimpl->subscribe(
                [ccfunction = std::move(cfunction),
                 subscriber,
                 state = std::move(cinitialState)]() mutable {
                    state = ccfunction(state);
                    subscriber.pushValue(state);
                },
                [subscriber](auto error) mutable { subscriber.pushError(error); },
                [subscriber]() mutable { subscriber.complete(); });
        });
}
} // namespace asyncly
