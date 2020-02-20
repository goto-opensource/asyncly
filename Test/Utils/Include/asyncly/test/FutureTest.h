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

#include <gmock/gmock.h>

#include "asyncly/executor/ThreadPoolExecutorController.h"

#include "WaitForFuture.h"

namespace asyncly {
namespace test {

// see FutureTest for documentation

/// FutureTest provides helper functions for using Futures in
/// GoogleTests. This is intended to be used for tests only, please
/// don't copy this code somewhere and use it in
/// production. Blockingly waiting for things defeats the whole point
/// of having futures in the first place.

class FutureTest : public ::testing::Test {
  public:
    FutureTest();

    /// wait_for_future blockingly waits for a Future given to it and
    /// returns the value the future resolves to. In case the future
    /// is rejected, the exception it is rejected with is thrown.
    template <typename T> T wait_for_future(asyncly::Future<T>&& future);

    /// wait_for_future_failure blockingly waits for a Future to
    /// fail. In case the future is unexpectedly resolved to a value,
    /// a std::runtime_error is thrown. If you are interested in the
    /// actual exception that's thrown, it's possible to use
    /// wait_for_future instead and surround it with a try/catch
    /// block.
    template <typename T> void wait_for_future_failure(asyncly::Future<T>&& future);

    /// collect_observable blockingly waits for an observable to complete and
    /// returns all gathered values in a vector. If the observable never completes, this
    /// function will block indefinitely. The parameter passed is a function because it
    /// internally needs to be executed in an executor context.
    /// Example usage:
    /// \snippet ExecutorProyTest.cpp Collect Observable
    template <typename F>
    std::vector<typename std::result_of<F()>::type::value_type>
    collect_observable(F functionReturningObservable);

  private:
    const std::unique_ptr<IExecutorController> waitExecutorController_;
    const std::shared_ptr<IExecutor> waitExecutor_;
};

inline FutureTest::FutureTest()
    : waitExecutorController_{ ThreadPoolExecutorController::create(1) }
    , waitExecutor_(waitExecutorController_->get_executor())
{
}

template <typename T> T FutureTest::wait_for_future(asyncly::Future<T>&& future)
{
    return asyncly::test::wait_for_future<T>(waitExecutor_, std::move(future));
}

template <typename T> void FutureTest::wait_for_future_failure(asyncly::Future<T>&& future)
{
    return asyncly::test::wait_for_future_failure<T>(waitExecutor_, std::move(future));
}

template <typename F>
std::vector<typename std::result_of<F()>::type::value_type>
FutureTest::collect_observable(F functionReturningObservable)
{
    return asyncly::test::collect_observable(waitExecutor_, std::move(functionReturningObservable));
}
}
}
