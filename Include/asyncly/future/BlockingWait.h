/*
 * Copyright 2021 LogMeIn
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

#include "asyncly/executor/InlineExecutor.h"
#include "asyncly/future/Future.h"
#include "asyncly/future/WhenAll.h"

#include <future>

namespace asyncly {
template <typename F> using FutureReturnValueType = typename std::result_of<F()>::type::value_type;

namespace detail {
template <typename F, typename R = void> struct BlockingWait;

template <typename F>
struct BlockingWait<
    F,
    typename std::enable_if<!std::is_same<void, FutureReturnValueType<F>>::value, void>::type> {
    FutureReturnValueType<F> operator()(const std::shared_ptr<IExecutor>& executor, const F& func)
    {
        using T = FutureReturnValueType<F>;
        std::promise<T> syncPromise;
        auto syncFuture = syncPromise.get_future();

        executor->post([&func, &syncPromise]() mutable {
            func()
                .then([&syncPromise](T result) { syncPromise.set_value(std::move(result)); })
                .catch_error(
                    [&syncPromise](std::exception_ptr e) { syncPromise.set_exception(e); });
        });

        return syncFuture.get();
    }
};

template <typename F>
struct BlockingWait<
    F,
    typename std::enable_if<std::is_same<void, FutureReturnValueType<F>>::value, void>::type> {
    FutureReturnValueType<F> operator()(const std::shared_ptr<IExecutor>& executor, const F& func)
    {
        std::promise<void> syncPromise;
        auto syncFuture = syncPromise.get_future();

        executor->post([&func, &syncPromise]() mutable {
            func()
                .then([&syncPromise]() { syncPromise.set_value(); })
                .catch_error(
                    [&syncPromise](std::exception_ptr e) { syncPromise.set_exception(e); });
        });

        syncFuture.get();
    }
};
} // namespace detail

/// Blockingly waits in the calling thread for the Future returning function to be executed in the
/// given executor.
/// \return the value the future resolves to. In case the future is rejected, the
/// exception it is rejected with is thrown.
/// Note: You should NEVER use this from within an asyncly executor because it may lead to a
/// deadlock! Main use case is for interop between a single-threaded piece of code (e.g. `main`) and
/// an asynchronous function.
template <typename F>
FutureReturnValueType<F> blocking_wait(const std::shared_ptr<IExecutor>& executor, const F& func)
{
    return detail::BlockingWait<F>{}(executor, func);
}

/// Blockingly waits in the calling thread for the given Future.
/// \return the value the future resolves to. In case the future is rejected, the exception it
/// is rejected with is thrown. Note: You should NEVER use this from within an asyncly executor
/// because it may lead to a deadlock! Main use case is for interop between a single-threaded
/// piece of code (e.g. `main`) and an asynchronous function (e.g. Proxy method).
template <typename T> T blocking_wait(asyncly::Future<T>&& future);

template <typename T> T blocking_wait(asyncly::Future<T>&& future)
{
    const auto executor = InlineExecutor::create();
    std::promise<T> syncPromise;
    auto syncFuture = syncPromise.get_future();

    executor->post([&future, &syncPromise]() mutable {
        future.then([&syncPromise](T result) { syncPromise.set_value(std::move(result)); })
            .catch_error([&syncPromise](std::exception_ptr e) { syncPromise.set_exception(e); });
    });

    return syncFuture.get();
}

template <typename... Args>
detail::when_all_return_types<Args...> blocking_wait_all(Future<Args>&&... args)
{
    const auto executor = InlineExecutor::create();
    std::promise<detail::when_all_return_types<Args...>> syncPromise;
    auto syncFuture = syncPromise.get_future();

    executor->post([&]() mutable {
        when_all(std::move(args)...)
            .then([&syncPromise](auto... result) {
                syncPromise.set_value(std::make_tuple(result...));
            })
            .catch_error([&syncPromise](std::exception_ptr e) { syncPromise.set_exception(e); });
    });

    return syncFuture.get();
}

template <> inline void blocking_wait<>(asyncly::Future<void>&& future)
{
    const auto executor = InlineExecutor::create();
    std::promise<void> syncPromise;
    auto syncFuture = syncPromise.get_future();

    executor->post([&future, &syncPromise]() mutable {
        future.then([&syncPromise]() { syncPromise.set_value(); })
            .catch_error([&syncPromise](std::exception_ptr e) { syncPromise.set_exception(e); });
    });

    syncFuture.get();
}
} // namespace asyncly
