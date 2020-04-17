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

#include "CurrentExecutorGuard.h"
#include "FakeExecutor.h"
#include "WaitForFuture.h"

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
    const std::unique_ptr<IExecutorController> waitExecutorController_;
    const std::shared_ptr<IExecutor> waitExecutor_;
    const asyncly::test::FakeExecutorPtr fakeExecutor_;
    const std::unique_ptr<asyncly::test::CurrentExecutorGuard> currentExecutorGuard_;
};

inline FakeFutureTest::FakeFutureTest()
    : waitExecutorController_{ ThreadPoolExecutorController::create(1) }
    , waitExecutor_(waitExecutorController_->get_executor())
    , fakeExecutor_(asyncly::test::FakeExecutor::create())
    , currentExecutorGuard_(std::make_unique<asyncly::test::CurrentExecutorGuard>(fakeExecutor_))
{
}

inline asyncly::test::FakeExecutorPtr FakeFutureTest::get_fake_executor()
{
    return fakeExecutor_;
}

template <typename T> T FakeFutureTest::wait_for_future(asyncly::Future<T>&& future)
{
    fakeExecutor_->runTasks();
    return asyncly::test::wait_for_future<T>(waitExecutor_, std::move(future));
}

template <typename T> void FakeFutureTest::wait_for_future_failure(asyncly::Future<T>&& future)
{
    fakeExecutor_->runTasks();
    return asyncly::test::wait_for_future_failure<T>(waitExecutor_, std::move(future));
}

}
}
