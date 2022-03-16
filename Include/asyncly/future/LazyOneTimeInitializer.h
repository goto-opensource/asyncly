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

#include "asyncly/future/Split.h"
#include "asyncly/future/detail/FutureTraits.h"

#include <boost/optional.hpp>

#include <type_traits>

namespace asyncly {

/// LazyOneTimeInitializer takes a function that returns a future. It allows getting multiple
/// futures for this return value while guaranteeing that the function will be invoked lazily and
/// only once when the first future is requested.
///
/// Example usage within a class:
/// \snippet LazyOneTimeInitializerTest.cpp LazyOneTimeInitializer WithinClass
template <typename T> class LazyOneTimeInitializer {
  public:
    template <typename F>
    LazyOneTimeInitializer(F&& fn)
        : _fn(std::forward<F>(fn))
    {
        assert(_fn && "The initialization function has to be valid!");

        using R = typename std::invoke_result<F>::type;
        static_assert(
            detail::future_traits<R>::is_future::value,
            "The function has to return an asyncly::Future!");
        using T2 = typename R::value_type;
        static_assert(std::is_same<T, T2>::value, "The return type Future type has to match T!");
    }

    /// \return A future holding the value returned from calling the provided function.
    /// The function is only ever called once irrespective of how often get() is called.
    /// This function is not thread-safe.
    asyncly::Future<T> get()
    {
        if (!_future) {
            _future.emplace(_fn());
            _fn = {};
        }
        auto pair = asyncly::split(std::move(*_future));
        _future.emplace(std::move(std::get<0>(pair)));
        return std::get<1>(pair);
    }

    bool hasFuture() const
    {
        assert(
            (!_fn != !_future)
            && "Invariant violated! There is always ever either the initializing function or the "
               "future it returned.");
        return _future.has_value();
    }

  private:
    std::function<asyncly::Future<T>()> _fn;

    boost::optional<asyncly::Future<T>> _future;
};

/// Creates a LazyOneTimeInitializer given a function returning an asyncly::Future.
///
/// Example usage within a class:
/// \snippet LazyOneTimeInitializerTest.cpp LazyOneTimeInitializer WithinClass
template <typename F> auto createLazyOneTimeInitializer(F&& fn)
{
    using R = typename std::invoke_result<F>::type;
    static_assert(
        detail::future_traits<R>::is_future::value,
        "The function has to return an asyncly::Future!");
    using T = typename R::value_type;

    return LazyOneTimeInitializer<T>(std::forward<F>(fn));
}
} // namespace asyncly
