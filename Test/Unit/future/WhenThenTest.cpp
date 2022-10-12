/*
 * Copyright 2022 LogMeIn
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

#include "asyncly/future/WhenThen.h"

#include "StrandImplTestFactory.h"
#include "asyncly/future/Future.h"
#include "asyncly/test/ExecutorTestFactories.h"

#include "gmock/gmock.h"

namespace asyncly {

using namespace testing;

template <typename TExecutorFactory> class WhenThenTest : public Test {
  public:
    WhenThenTest()
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

TYPED_TEST_SUITE(WhenThenTest, ExecutorFactoryTypes);

TYPED_TEST(WhenThenTest, shouldResolveValueFutures)
{
    std::promise<int> resolved;

    this->executor_->post([&resolved]() {
        auto lazy1 = make_lazy_future<int>();
        auto future1 = std::get<0>(lazy1);
        auto promise1 = std::get<1>(lazy1);
        auto lazy2 = make_lazy_future<int>();
        auto future2 = std::get<0>(lazy2);
        auto promise2 = std::get<1>(lazy2);

        when_then(std::move(future1), promise2);

        future2.then([&resolved](int i) { resolved.set_value(i); }).catch_error([](auto) {
            ADD_FAILURE();
        });
        promise1.set_value(5);
    });

    EXPECT_EQ(5, resolved.get_future().get());
}

TYPED_TEST(WhenThenTest, shouldResolveVoidFutures)
{
    std::promise<void> resolved;

    this->executor_->post([&resolved]() {
        auto lazy1 = make_lazy_future<void>();
        auto future1 = std::get<0>(lazy1);
        auto promise1 = std::get<1>(lazy1);
        auto lazy2 = make_lazy_future<void>();
        auto future2 = std::get<0>(lazy2);
        auto promise2 = std::get<1>(lazy2);

        when_then(std::move(future1), promise2);

        future2.then([&resolved]() { resolved.set_value(); }).catch_error([](auto) {
            ADD_FAILURE();
        });
        promise1.set_value();
    });

    resolved.get_future().get();
}

TYPED_TEST(WhenThenTest, shouldRejectValueFutures)
{
    std::promise<int> resolved;

    this->executor_->post([&resolved]() {
        auto lazy1 = make_lazy_future<int>();
        auto future1 = std::get<0>(lazy1);
        auto promise1 = std::get<1>(lazy1);
        auto lazy2 = make_lazy_future<int>();
        auto future2 = std::get<0>(lazy2);
        auto promise2 = std::get<1>(lazy2);

        when_then(std::move(future1), promise2);

        std::exception_ptr e;
        try {
            throw std::runtime_error("test error");
        } catch (...) {
            e = std::current_exception();
        }

        future2.then([](auto) { ADD_FAILURE(); }).catch_error([&resolved](auto e) {
            resolved.set_exception(e);
        });
        promise1.set_exception(e);
    });

    EXPECT_THROW(resolved.get_future().get(), std::runtime_error);
}

TYPED_TEST(WhenThenTest, shouldRejectVoidFutures)
{
    std::promise<void> resolved;

    this->executor_->post([&resolved]() {
        auto lazy1 = make_lazy_future<void>();
        auto future1 = std::get<0>(lazy1);
        auto promise1 = std::get<1>(lazy1);
        auto lazy2 = make_lazy_future<void>();
        auto future2 = std::get<0>(lazy2);
        auto promise2 = std::get<1>(lazy2);

        when_then(std::move(future1), promise2);

        std::exception_ptr e;
        try {
            throw std::runtime_error("test error");
        } catch (...) {
            e = std::current_exception();
        }

        future2.then([]() { ADD_FAILURE(); }).catch_error([&resolved](auto e) {
            resolved.set_exception(e);
        });
        promise1.set_exception(e);
    });

    EXPECT_THROW(resolved.get_future().get(), std::runtime_error);
}
}