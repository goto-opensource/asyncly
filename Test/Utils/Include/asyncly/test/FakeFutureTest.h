/*
 * Copyright 2020 LogMeIn
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

#include <gmock/gmock.h>

#include "asyncly/executor/ThreadPoolExecutorController.h"
#include "asyncly/future/Future.h"

#include "CurrentExecutorGuard.h"
#include "FakeExecutor.h"

#include <future>

namespace asyncly {
namespace test {

// see FutureTest for documentation

/// FakeFutureTest provides helper functions for using Futures in
/// GoogleTests with a FakeExecutor.

class FakeFutureTest : public ::testing::Test {
  public:
    FakeFutureTest();

    /// wait_for_future blockingly waits for a Future given to it and
    /// returns the value the future resolves to. In case the future
    /// is rejected, the exception it is rejected with is thrown.
    /// All tasks of the FakeExecutor are run before the future handling
    /// is triggered.
    template <typename T> T wait_for_future(asyncly::Future<T>&& future);

    /// wait_for_future_failure blockingly waits for a Future to
    /// fail. In case the future is unexpectedly resolved to a value,
    /// a std::runtime_error is thrown. If you are interested in the
    /// actual exception that's thrown, it's possible to use
    /// wait_for_future instead and surround it with a try/catch
    /// block. All tasks of the FakeExecutor are run before the
    /// future handling is triggered.
    template <typename T> void wait_for_future_failure(asyncly::Future<T>&& future);

    /// Returns used FakeExecutor
    asyncly::test::FakeExecutorPtr get_fake_executor();

  private:
    const asyncly::test::FakeExecutorPtr fakeExecutor_;
    const std::unique_ptr<asyncly::test::CurrentExecutorGuard> currentExecutorGuard_;
};

inline FakeFutureTest::FakeFutureTest()
    : fakeExecutor_(asyncly::test::FakeExecutor::create())
    , currentExecutorGuard_(std::make_unique<asyncly::test::CurrentExecutorGuard>(fakeExecutor_))
{
}

inline asyncly::test::FakeExecutorPtr FakeFutureTest::get_fake_executor()
{
    return fakeExecutor_;
}

template <typename T> T FakeFutureTest::wait_for_future(asyncly::Future<T>&& future)
{
    std::promise<T> syncPromise;
    auto syncFuture = syncPromise.get_future();

    future.then([&syncPromise](T result) { syncPromise.set_value(std::move(result)); })
        .catch_error([&syncPromise](std::exception_ptr e) { syncPromise.set_exception(e); });

    fakeExecutor_->runTasks();

    return syncFuture.get();
}

template <> inline void FakeFutureTest::wait_for_future<>(asyncly::Future<void>&& future)
{
    std::promise<void> syncPromise;
    auto syncFuture = syncPromise.get_future();

    future.then([&syncPromise]() { syncPromise.set_value(); })
        .catch_error([&syncPromise](std::exception_ptr e) { syncPromise.set_exception(e); });

    fakeExecutor_->runTasks();

    syncFuture.get();
}

template <typename T> void FakeFutureTest::wait_for_future_failure(asyncly::Future<T>&& future)
{
    std::promise<void> syncPromise;
    auto syncFuture = syncPromise.get_future();

    future
        .then([&syncPromise](T) {
            syncPromise.set_exception(std::make_exception_ptr(
                std::runtime_error{ "Expected failure, but found success" }));
        })
        .catch_error([&syncPromise](std::exception_ptr) { syncPromise.set_value(); });

    fakeExecutor_->runTasks();

    syncFuture.get();
}

template <> inline void FakeFutureTest::wait_for_future_failure<>(asyncly::Future<void>&& future)
{
    std::promise<void> syncPromise;
    auto syncFuture = syncPromise.get_future();

    future
        .then([&syncPromise]() {
            syncPromise.set_exception(std::make_exception_ptr(
                std::runtime_error{ "Expected failure, but found success" }));
        })
        .catch_error([&syncPromise](std::exception_ptr) { syncPromise.set_value(); });

    fakeExecutor_->runTasks();

    syncFuture.get();
}

}
}
