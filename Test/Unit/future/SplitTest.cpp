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

#include "asyncly/future/Split.h"

#include "asyncly/test/ExecutorTestFactories.h"

#include "gmock/gmock.h"

namespace asyncly {

using namespace testing;

template <typename TExecutorFactory> class SplitTest : public Test {
  public:
    SplitTest()
        : factory_(std::make_unique<TExecutorFactory>())
        , executor_(factory_->create())
    {
    }

    std::unique_ptr<TExecutorFactory> factory_;
    std::shared_ptr<IExecutor> executor_;
};

using ExecutorFactoryTypes = ::testing::Types<
    asyncly::test::AsioExecutorFactory<>,
    asyncly::test::DefaultExecutorFactory<>,
    asyncly::test::StrandFactory<>>;

TYPED_TEST_SUITE(SplitTest, ExecutorFactoryTypes);

TYPED_TEST(SplitTest, shouldSplitVoidFutures)
{
    std::promise<void> called1;
    std::promise<void> called2;
    this->executor_->post([&called1, &called2]() {
        auto lazy = make_lazy_future<void>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        auto twoFutures = asyncly::split(std::move(future));
        auto future1 = std::get<0>(twoFutures);
        auto future2 = std::get<1>(twoFutures);

        future1.then([&called1]() { called1.set_value(); });
        future2.then([&called2]() { called2.set_value(); });

        promise.set_value();
    });

    called1.get_future().wait();
    called2.get_future().wait();
}

TYPED_TEST(SplitTest, shouldSplitFutures)
{
    std::promise<int> called1;
    std::promise<int> called2;
    auto value = 5;
    this->executor_->post([&called1, &called2, value]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        auto twoFutures = asyncly::split(std::move(future));
        auto future1 = std::get<0>(twoFutures);
        auto future2 = std::get<1>(twoFutures);

        future1.then([&called1](int v) { called1.set_value(v); });
        future2.then([&called2](int v) { called2.set_value(v); });

        promise.set_value(value);
    });

    EXPECT_EQ(called1.get_future().get(), value);
    EXPECT_EQ(called2.get_future().get(), value);
}

TYPED_TEST(SplitTest, shouldSplitErrorFutures)
{
    std::promise<int> called1;
    std::promise<int> called2;

    struct MyError : public std::exception {
    };

    this->executor_->post([&called1, &called2]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        auto twoFutures = asyncly::split(std::move(future));
        auto future1 = std::get<0>(twoFutures);
        auto future2 = std::get<1>(twoFutures);

        future1.then([](int) {}).catch_error([&called1](auto e) { called1.set_exception(e); });
        future2.then([](int) {}).catch_error([&called2](auto e) { called2.set_exception(e); });

        promise.set_exception(MyError{});
    });

    EXPECT_THROW(called1.get_future().get(), MyError);
    EXPECT_THROW(called2.get_future().get(), MyError);
}

TYPED_TEST(SplitTest, shouldSplitErrorVoidFutures)
{
    std::promise<void> called1;
    std::promise<void> called2;

    struct MyError : public std::exception {
    };

    this->executor_->post([&called1, &called2]() {
        auto lazy = make_lazy_future<void>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        auto twoFutures = asyncly::split(std::move(future));
        auto future1 = std::get<0>(twoFutures);
        auto future2 = std::get<1>(twoFutures);

        future1.then([]() {}).catch_error([&called1](auto e) { called1.set_exception(e); });
        future2.then([]() {}).catch_error([&called2](auto e) { called2.set_exception(e); });

        promise.set_exception(MyError{});
    });

    EXPECT_THROW(called1.get_future().get(), MyError);
    EXPECT_THROW(called2.get_future().get(), MyError);
}
}
