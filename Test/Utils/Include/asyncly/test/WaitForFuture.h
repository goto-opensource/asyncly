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

#include "asyncly/ExecutorTypes.h"
#include "asyncly/future/Future.h"
#include "asyncly/observable/Observable.h"

#include <exception>
#include <future>
#include <vector>

namespace asyncly::test {

/// Blockingly waits in the calling thread for the Future given and returns the value the future
/// resolves to. In case the future is rejected, the exception it is rejected with is thrown.
template <typename T>
T wait_for_future(const std::shared_ptr<IExecutor>& executor, asyncly::Future<T>&& future);

template <typename T>
T wait_for_future(const std::shared_ptr<IExecutor>& executor, asyncly::Future<T>&& future)
{
    std::promise<T> syncPromise;
    auto syncFuture = syncPromise.get_future();

    executor->post([&future, &syncPromise]() mutable {
        future.then([&syncPromise](T result) { syncPromise.set_value(std::move(result)); })
            .catch_error([&syncPromise](std::exception_ptr e) { syncPromise.set_exception(e); });
    });

    return syncFuture.get();
}

template <>
inline void
wait_for_future<>(const std::shared_ptr<IExecutor>& executor, asyncly::Future<void>&& future)
{
    std::promise<void> syncPromise;
    auto syncFuture = syncPromise.get_future();

    executor->post([&future, &syncPromise]() mutable {
        future.then([&syncPromise]() { syncPromise.set_value(); })
            .catch_error([&syncPromise](std::exception_ptr e) { syncPromise.set_exception(e); });
    });

    syncFuture.get();
}

/// Blockingly waits for the Future to fail. In case the future is unexpectedly resolved to a value,
/// a std::runtime_error is thrown. If you are interested in the actual exception that's thrown,
/// it's possible to use wait_for_future instead and surround it with a try/catch block.
template <typename T>
void wait_for_future_failure(
    const std::shared_ptr<IExecutor>& executor, asyncly::Future<T>&& future);

template <typename T>
void wait_for_future_failure(
    const std::shared_ptr<IExecutor>& executor, asyncly::Future<T>&& future)
{
    std::promise<void> syncPromise;
    auto syncFuture = syncPromise.get_future();

    executor->post([&future, &syncPromise]() mutable {
        future
            .then([&syncPromise](T) {
                syncPromise.set_exception(std::make_exception_ptr(
                    std::runtime_error{ "Expected failure, but found success" }));
            })
            .catch_error([&syncPromise](std::exception_ptr) { syncPromise.set_value(); });
    });

    syncFuture.get();
}

template <>
inline void wait_for_future_failure<>(
    const std::shared_ptr<IExecutor>& executor, asyncly::Future<void>&& future)
{
    std::promise<void> syncPromise;
    auto syncFuture = syncPromise.get_future();

    executor->post([&future, &syncPromise]() mutable {
        future
            .then([&syncPromise]() {
                syncPromise.set_exception(std::make_exception_ptr(
                    std::runtime_error{ "Expected failure, but found success" }));
            })
            .catch_error([&syncPromise](std::exception_ptr) { syncPromise.set_value(); });
    });

    syncFuture.get();
}

/// Blockingly waits for an observable to complete and returns all gathered values in a vector. If
/// the observable never completes, this function will block indefinitely. The parameter passed is a
/// function because it internally needs to be executed in an executor context.
template <typename F>
std::vector<typename std::result_of<F()>::type::value_type>
collect_observable(const std::shared_ptr<IExecutor>& executor, F functionReturningObservable);

template <typename F>
std::vector<typename std::result_of<F()>::type::value_type>
collect_observable(const asyncly::IExecutorPtr& executor, F functionReturningObservable)
{
    using T = typename std::result_of<F()>::type::value_type;
    std::promise<void> syncPromise;
    auto syncFuture = syncPromise.get_future();

    std::vector<T> result;

    executor->post([&syncPromise, &result, &functionReturningObservable]() mutable {
        auto observable = functionReturningObservable();
        observable.subscribe(
            [&result](T value) { result.push_back(value); },
            [&syncPromise](std::exception_ptr e) { syncPromise.set_exception(e); },
            [&syncPromise]() { syncPromise.set_value(); });
    });

    syncFuture.get();

    return result;
}
} // namespace asyncly::test
