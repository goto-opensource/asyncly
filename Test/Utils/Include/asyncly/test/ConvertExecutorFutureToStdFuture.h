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

#include <future>

namespace asyncly::test {
template <typename T>
std::future<T> convertExecutorFutureToStdFuture(asyncly::Future<T>& executorFuture)
{
    auto promise = std::make_shared<std::promise<T>>();
    auto future = promise->get_future();
    executorFuture.then([promise](T val) mutable { promise->set_value(std::move(val)); })
        .catch_error([promise](std::exception_ptr e) { promise->set_exception(e); });
    return future;
}
template <typename T>
std::future<T> convertExecutorFutureToStdFuture(asyncly::Future<T>&& executorFuture)
{
    auto promise = std::make_shared<std::promise<T>>();
    auto future = promise->get_future();
    executorFuture.then([promise](T val) mutable { promise->set_value(std::move(val)); })
        .catch_error([promise](std::exception_ptr e) { promise->set_exception(e); });
    return future;
}
template <>
inline std::future<void> convertExecutorFutureToStdFuture(asyncly::Future<void>& executorFuture)
{
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();
    executorFuture.then([promise]() mutable { promise->set_value(); })
        .catch_error([promise](std::exception_ptr e) { promise->set_exception(e); });
    return future;
}
template <>
inline std::future<void> convertExecutorFutureToStdFuture(asyncly::Future<void>&& executorFuture)
{
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();
    executorFuture.then([promise]() mutable { promise->set_value(); })
        .catch_error([promise](std::exception_ptr e) { promise->set_exception(e); });
    return future;
}
} // namespace asyncly::test
