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

#include "asyncly/future/Future.h"

namespace asyncly {

/// split produces two Future<T> from a single one. This can be used to represent forks in
/// asynchronous computation graphs. This only works if T is copyable, as we have to deliver it two
/// destinations. This function consumes the future supplied to it.
template <typename T> std::tuple<Future<T>, Future<T>> split(Future<T>&& future)
{
    static_assert(
        std::is_copy_constructible_v<T>, "You can only split futures for copy-constructible types");
    auto lazy1 = make_lazy_future<T>();
    auto future1 = std::get<0>(lazy1);
    auto promise1 = std::get<1>(lazy1);

    auto lazy2 = make_lazy_future<T>();
    auto future2 = std::get<0>(lazy2);
    auto promise2 = std::get<1>(lazy2);

    std::move(future)
        .then([promise1, promise2](T value) mutable {
            promise1.set_value(value);
            promise2.set_value(value);
        })
        .catch_error([promise1, promise2](auto error) mutable {
            promise1.set_exception(error);
            promise2.set_exception(error);
        });

    return std::make_tuple(future1, future2);
}

template <> inline std::tuple<Future<void>, Future<void>> split(Future<void>&& future)
{
    auto lazy1 = make_lazy_future<void>();
    auto future1 = std::get<0>(lazy1);
    auto promise1 = std::get<1>(lazy1);

    auto lazy2 = make_lazy_future<void>();
    auto future2 = std::get<0>(lazy2);
    auto promise2 = std::get<1>(lazy2);

    std::move(future)
        .then([promise1, promise2]() mutable {
            promise1.set_value();
            promise2.set_value();
        })
        .catch_error([promise1, promise2](auto error) mutable {
            promise1.set_exception(error);
            promise2.set_exception(error);
        });

    return std::make_tuple(future1, future2);
}
} // namespace asyncly
