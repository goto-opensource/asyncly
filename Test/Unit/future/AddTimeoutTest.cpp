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

#include "asyncly/future/AddTimeout.h"

#include "StrandImplTestFactory.h"
#include "asyncly/test/ExecutorTestFactories.h"

#include "gmock/gmock.h"

#include <chrono>

namespace asyncly {

using namespace testing;

template <typename TExecutorFactory> class AddTimeoutTest : public Test {
  public:
    AddTimeoutTest()
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

TYPED_TEST_SUITE(AddTimeoutTest, ExecutorFactoryTypes);

TYPED_TEST(AddTimeoutTest, shouldTimeoutVoidFuturesWithImmediateTimeout)
{
    std::promise<void> timedOut;
    this->executor_->post([&timedOut]() {
        auto lazy = make_lazy_future<void>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        add_timeout(std::chrono::milliseconds(0), std::move(future))
            .then([]() { FAIL(); })
            .catch_error([&timedOut](auto error) mutable {
                try {
                    std::rethrow_exception(error);
                } catch (const Timeout&) {
                    timedOut.set_value();
                } catch (const std::exception&) {
                    FAIL();
                } catch (...) {
                    FAIL();
                }
            });
    });

    EXPECT_NO_THROW(timedOut.get_future().get());
}

TYPED_TEST(AddTimeoutTest, shouldTimeoutValueFuturesWithImmediateTimeout)
{
    std::promise<int> timedOut;
    this->executor_->post([&timedOut]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        add_timeout(std::chrono::milliseconds(0), std::move(future))
            .then([](auto) { FAIL(); })
            .catch_error([&timedOut](auto error) mutable {
                try {
                    std::rethrow_exception(error);
                } catch (const asyncly::Timeout& e) {
                    timedOut.set_exception(std::make_exception_ptr(e));
                } catch (const std::exception&) {
                    FAIL();
                } catch (...) {
                    FAIL();
                }
            });
    });

    EXPECT_THROW(timedOut.get_future().get(), asyncly::Timeout);
}

TYPED_TEST(AddTimeoutTest, shouldNotTimeoutVoidFuturesWithIndefiniteTimeout)
{
    std::promise<void> succeeded;
    this->executor_->post([&succeeded]() {
        auto lazy = make_lazy_future<void>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        add_timeout(std::chrono::hours(100), std::move(future))
            .then([&succeeded]() { succeeded.set_value(); })
            .catch_error([](auto error) {
                try {
                    std::rethrow_exception(error);
                } catch (...) {
                    FAIL();
                }
            });
        promise.set_value();
    });

    EXPECT_NO_THROW(succeeded.get_future().get());
}

TYPED_TEST(AddTimeoutTest, shouldNotTimeoutValueFuturesWithIndefiniteTimeout)
{
    auto value = 5;
    std::promise<int> succeeded;
    this->executor_->post([&succeeded, value]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        add_timeout(std::chrono::hours(100), std::move(future))
            .then([&succeeded](int v) { succeeded.set_value(v); })
            .catch_error([](auto error) {
                try {
                    std::rethrow_exception(error);
                } catch (...) {
                    FAIL();
                }
            });
        promise.set_value(value);
    });

    EXPECT_EQ(succeeded.get_future().get(), 5);
}

TYPED_TEST(AddTimeoutTest, shouldNotTimeoutErrorVoidFuturesWithIndefiniteTimeout)
{
    struct MyError : public std::exception {
    };

    std::promise<void> succeeded;
    this->executor_->post([&succeeded]() {
        auto lazy = make_lazy_future<void>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        add_timeout(std::chrono::hours(100), std::move(future))
            .then([]() {})
            .catch_error([&succeeded](auto error) {
                try {
                    std::rethrow_exception(error);
                } catch (const MyError& e) {
                    succeeded.set_exception(std::make_exception_ptr(e));
                } catch (...) {
                    FAIL();
                }
            });
        promise.set_exception(MyError{});
    });

    EXPECT_THROW(succeeded.get_future().get(), MyError);
}

TYPED_TEST(AddTimeoutTest, shouldNotTimeoutErrorValueFuturesWithIndefiniteTimeout)
{
    struct MyError : public std::exception {
    };

    std::promise<int> succeeded;
    this->executor_->post([&succeeded]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        add_timeout(std::chrono::hours(100), std::move(future))
            .then([](auto) {})
            .catch_error([&succeeded](auto error) {
                try {
                    std::rethrow_exception(error);
                } catch (const MyError& e) {
                    succeeded.set_exception(std::make_exception_ptr(e));
                } catch (...) {
                    FAIL();
                }
            });
        promise.set_exception(MyError{});
    });

    EXPECT_THROW(succeeded.get_future().get(), MyError);
}

} // namespace asyncly
