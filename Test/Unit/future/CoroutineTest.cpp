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

#include "asyncly/future/Future.h"

#include "StrandImplTestFactory.h"
#include "asyncly/test/ExecutorTestFactories.h"

#include "gmock/gmock.h"

namespace asyncly {

using namespace testing;

template <typename TExecutorFactory> class CoroutineTest : public Test {
  public:
    CoroutineTest()
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
    asyncly::test::StrandImplTestFactory<>>;

TYPED_TEST_SUITE(CoroutineTest, ExecutorFactoryTypes);

#ifdef ASYNCLY_HAS_COROUTINES
TYPED_TEST(CoroutineTest, shouldSupportCoroutineAsyncAwaitOfReadyFuture)
{
    auto value = std::make_shared<std::promise<int>>();

    this->executor_->post([&value]() -> Future<void> {
        // todo: prolong task life time througout suspensions
        auto value2 = value;
        auto result = co_await make_ready_future(42);
        value2->set_value(result);
    });

    EXPECT_EQ(42, value->get_future().get());
}

TYPED_TEST(CoroutineTest, shouldSupportCoroutineAsyncAwaitOfReadyVoidFuture)
{
    auto value = std::make_shared<std::promise<void>>();

    this->executor_->post([&value]() -> Future<void> {
        // todo: prolong task life time througout suspensions
        auto value2 = value;
        co_await make_ready_future();
        value2->set_value();
    });

    value->get_future().get();
}

TYPED_TEST(CoroutineTest, shouldSupportCoroutineAsyncAwaitOfLazyFuture)
{
    auto value = std::make_shared<std::promise<int>>();

    auto lazy = make_lazy_future<int>();
    auto future = std::get<0>(lazy);
    auto promise = std::get<1>(lazy);

    this->executor_->post([&value, &future]() -> Future<void> {
        // todo: prolong task life time througout suspensions
        auto value2 = value;
        auto result = co_await future;

#ifdef ASYNCLY_FUTURE_DEBUG
        std::cerr << "co_await ready, value is " << result << " on thread "
                  << std::this_thread::get_id() << std::endl;
#endif

        value2->set_value(result);
    });

    this->executor_->post([&promise]() {
        auto value = 42;
#ifdef ASYNCLY_FUTURE_DEBUG
        std::cerr << "promise.set_value(" << value << ")"
                  << " on thread " << std::this_thread::get_id() << std::endl;
#endif
        promise.set_value(std::move(value));
    });
    EXPECT_EQ(42, value->get_future().get());
}

TYPED_TEST(CoroutineTest, shouldSupportCoroutineAsyncAwaitOfLazyVoidFuture)
{
    auto value = std::make_shared<std::promise<void>>();

    auto lazy = make_lazy_future<void>();
    auto future = std::get<0>(lazy);
    auto promise = std::get<1>(lazy);

    this->executor_->post([&value, &future]() -> Future<void> {
        // todo: prolong task life time througout suspensions
        auto value2 = value;
        co_await future;

#ifdef ASYNCLY_FUTURE_DEBUG
        std::cerr << "co_await ready, on thread " << std::this_thread::get_id() << std::endl;
#endif

        value2->set_value();
    });

    this->executor_->post([&promise]() {
#ifdef ASYNCLY_FUTURE_DEBUG
        std::cerr << "promise.set_value()"
                  << " on thread " << std::this_thread::get_id() << std::endl;
#endif
        promise.set_value();
    });
    value->get_future().get();
}

TYPED_TEST(CoroutineTest, shouldSupportCoroutineAsyncAwaitOfExceptionalFuture)
{
    auto value = std::make_shared<std::promise<int>>();

    this->executor_->post([&value]() -> Future<void> {
        // todo: prolong task life time througout suspensions
        auto value2 = value;
        try {
            co_await make_exceptional_future<int>("intentaional error");
        } catch (...) {
            value2->set_exception(std::current_exception());
        }
    });

    EXPECT_ANY_THROW(value->get_future().get());
}

TYPED_TEST(CoroutineTest, shouldSupportCoroutineAsyncAwaitOfExceptionalVoidFuture)
{
    auto value = std::make_shared<std::promise<void>>();

    this->executor_->post([&value]() -> Future<void> {
        // todo: prolong task life time througout suspensions
        auto value2 = value;
        try {
            co_await make_exceptional_future<void>("intentaional error");
        } catch (...) {
            value2->set_exception(std::current_exception());
        }
    });

    EXPECT_ANY_THROW(value->get_future().get());
}

TYPED_TEST(CoroutineTest, shouldThrowOnSecondCoroutineAsyncAwaitOfReadyFuture)
{
    auto value = std::make_shared<std::promise<int>>();
    auto future = make_ready_future(42);

    this->executor_->post([&value, &future]() -> Future<void> {
        // todo: prolong task life time througout suspensions
        auto value2 = value;
        auto future2 = future;
        co_await future2;

        try {
            co_await future2;
        } catch (...) {
            value2->set_exception(std::current_exception());
        }
    });

    EXPECT_ANY_THROW(value->get_future().get());
}

TYPED_TEST(CoroutineTest, shouldThrowOnSecondCoroutineAsyncAwaitOfReadyVoidFuture)
{
    auto value = std::make_shared<std::promise<void>>();
    auto future = make_ready_future();

    this->executor_->post([&value, &future]() -> Future<void> {
        // todo: prolong task life time througout suspensions
        auto value2 = value;
        auto future2 = future;
        co_await future2;

        try {
            co_await future2;
        } catch (...) {
            value2->set_exception(std::current_exception());
        }
    });

    EXPECT_ANY_THROW(value->get_future().get());
}
#endif
}
