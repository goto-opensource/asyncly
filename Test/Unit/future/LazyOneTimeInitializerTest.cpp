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

#include "asyncly/future/LazyOneTimeInitializer.h"

#include "asyncly/test/ExecutorTestFactories.h"

#include "gmock/gmock.h"

namespace asyncly {

using namespace testing;

template <typename TExecutorFactory> class LazyOneTimeInitializerTest : public Test {
  public:
    LazyOneTimeInitializerTest()
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

TYPED_TEST_SUITE(LazyOneTimeInitializerTest, ExecutorFactoryTypes);

TYPED_TEST(LazyOneTimeInitializerTest, should_LazyOneTimeInitializer_haveNoFutureInitially)
{
    const auto lazyOneTimeInitializer
        = createLazyOneTimeInitializer([]() { return make_ready_future(42); });

    EXPECT_FALSE(lazyOneTimeInitializer.hasFuture());
}

TYPED_TEST(LazyOneTimeInitializerTest, should_LazyOneTimeInitializer_haveFutureAfterGetCalled)
{
    auto lazyOneTimeInitializer
        = createLazyOneTimeInitializer([]() { return make_ready_future(42); });

    std::promise<void> getCalled;
    this->executor_->post([&lazyOneTimeInitializer, &getCalled]() {
        lazyOneTimeInitializer.get();
        getCalled.set_value();
    });
    getCalled.get_future().get();

    EXPECT_TRUE(lazyOneTimeInitializer.hasFuture());
}

TYPED_TEST(
    LazyOneTimeInitializerTest,
    should_LazyOneTimeInitializer_ResolveAllFuturesGotAfterReadyAndCallFnOnlyOnce)
{
    int i = 0;
    auto lazyOneTimeInitializer
        = createLazyOneTimeInitializer([&i]() { return make_ready_future(42 + i++); });

    for (int j = 0; j < 3; ++j) {
        std::promise<int> value;
        this->executor_->post([&lazyOneTimeInitializer, &value]() {
            lazyOneTimeInitializer.get().then([&value](int v) { value.set_value(v); });
        });
        EXPECT_EQ(42, value.get_future().get());
    }
}

TYPED_TEST(LazyOneTimeInitializerTest, should_LazyOneTimeInitializer_RejectAllFuturesGotAfterReady)
{
    auto lazyOneTimeInitializer = createLazyOneTimeInitializer(
        []() { return make_exceptional_future<int>("intentional error"); });

    for (int i = 0; i < 3; ++i) {
        std::promise<void> value;
        this->executor_->post([&lazyOneTimeInitializer, &value]() {
            lazyOneTimeInitializer.get().catch_error([&value](auto) { value.set_value(); });
        });
        value.get_future().get();
    }
}

TYPED_TEST(
    LazyOneTimeInitializerTest, should_LazyOneTimeInitializer_ResolveAllFuturesGotBeforeReady)
{
    auto lazy = make_lazy_future<int>();
    auto future = std::get<0>(lazy);
    auto promise = std::get<1>(lazy);

    auto lazyOneTimeInitializer = createLazyOneTimeInitializer([&future]() { return future; });

    std::vector<std::future<int>> futures;

    for (int i = 0; i < 3; ++i) {
        std::promise<void> getCalled;
        std::promise<int> valueSet;
        futures.push_back(valueSet.get_future());
        this->executor_->post(
            [&lazyOneTimeInitializer, &getCalled, valueSet{ std::move(valueSet) }]() mutable {
                lazyOneTimeInitializer.get().then(
                    [valueSet{ std::move(valueSet) }](int v) mutable { valueSet.set_value(v); });
                getCalled.set_value();
            });
        getCalled.get_future().get();
        EXPECT_EQ(std::future_status::timeout, futures.back().wait_for(std::chrono::seconds(0u)));
    }

    promise.set_value(42);

    for (auto& fut : futures) {
        EXPECT_EQ(42, fut.get());
    }
}

TYPED_TEST(LazyOneTimeInitializerTest, should_LazyOneTimeInitializer_RejectAllFuturesGotBeforeReady)
{
    auto lazy = make_lazy_future<int>();
    auto future = std::get<0>(lazy);
    auto promise = std::get<1>(lazy);

    auto lazyOneTimeInitializer = createLazyOneTimeInitializer([&future]() { return future; });

    std::vector<std::future<void>> futures;

    for (int i = 0; i < 3; ++i) {
        std::promise<void> getCalled;
        std::promise<void> errorSet;
        futures.push_back(errorSet.get_future());
        this->executor_->post(
            [&lazyOneTimeInitializer, &getCalled, errorSet{ std::move(errorSet) }]() mutable {
                lazyOneTimeInitializer.get().catch_error(
                    [errorSet{ std::move(errorSet) }](auto) mutable { errorSet.set_value(); });
                getCalled.set_value();
            });
        getCalled.get_future().get();
        EXPECT_EQ(std::future_status::timeout, futures.back().wait_for(std::chrono::seconds(0u)));
    }

    promise.set_exception("intentional error");

    for (auto& fut : futures) {
        fut.get();
    }
}

//! [LazyOneTimeInitializer WithinClass]
class LazyOneTimeInitializerWithinClass {
  public:
    LazyOneTimeInitializerWithinClass()
        : _lazyOneTimeInitializer([]() { return make_ready_future(42); })
    {
    }

    asyncly::Future<int> do42Plus2()
    {
        return _lazyOneTimeInitializer.get().then([](auto i) { return i + 2; });
    }

    asyncly::Future<int> do42Minus2()
    {
        return _lazyOneTimeInitializer.get().then([](auto i) { return i - 2; });
    }

  private:
    LazyOneTimeInitializer<int> _lazyOneTimeInitializer;
};
//! [LazyOneTimeInitializer WithinClass]

TYPED_TEST(LazyOneTimeInitializerTest, should_LazyOneTimeInitializer_WithinClass)
{
    LazyOneTimeInitializerWithinClass dummy;

    std::promise<int> value0;
    this->executor_->post(
        [&dummy, &value0]() { dummy.do42Plus2().then([&value0](int v) { value0.set_value(v); }); });
    EXPECT_EQ(42 + 2, value0.get_future().get());

    std::promise<int> value1;
    this->executor_->post([&dummy, &value1]() {
        dummy.do42Minus2().then([&value1](int v) { value1.set_value(v); });
    });
    EXPECT_EQ(42 - 2, value1.get_future().get());
}
}
