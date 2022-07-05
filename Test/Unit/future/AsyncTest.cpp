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

#include "asyncly/future/Future.h"
#include "gmock/gmock.h"
#include <chrono>
#include <exception>
#include <memory>
#include <stdexcept>
#include <thread>

#include "StrandImplTestFactory.h"
#include "asyncly/test/ExecutorTestFactories.h"

namespace asyncly {

using namespace testing;

template <typename TExecutorFactory> class AsyncTest : public Test {
  public:
    AsyncTest()
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

TYPED_TEST_SUITE(AsyncTest, ExecutorFactoryTypes);

template <class Fn> void run_in_executor(std::shared_ptr<IExecutor> executor, Fn fn)
{
    auto done = std::make_shared<std::promise<void>>();
    auto future = done->get_future();
    executor->post([fn, done = std::move(done)]() { fn(done); });
    future.get();
}

TYPED_TEST(AsyncTest, async_returnValueWithNoParameters)
{
    run_in_executor(this->executor_, [&](auto done) {
        auto future = asyncly::async(this->executor_, []() { return 1; });
        future.then([done](int result) {
            EXPECT_EQ(result, 1);
            done->set_value();
        });
    });
}

TYPED_TEST(AsyncTest, do_returnValueWithParametersAsValue)
{
    run_in_executor(this->executor_, [&](auto done) {
        auto future = asyncly::async(
            this->executor_, [](int a, int b) { return a + b; }, 5, 7);
        future.then([done](int result) {
            EXPECT_EQ(result, 12);
            done->set_value();
        });
    });
}

TYPED_TEST(AsyncTest, do_returnValueWithParametersAsConstLRef)
{
    const int i = 5;
    const int j = 7;
    run_in_executor(this->executor_, [&](auto done) {
        auto future = asyncly::async(
            this->executor_, [](const int& a, const int& b) { return a + b; }, i, j);
        future.then([done](int result) {
            EXPECT_EQ(result, 12);
            done->set_value();
        });
    });
}

TYPED_TEST(AsyncTest, do_returnValueWithParametersLiteralsAsConstLRef)
{
    run_in_executor(this->executor_, [&](auto done) {
        auto future = asyncly::async(
            this->executor_, [](const int& a, const int& b) { return a + b; }, 5, 7);
        future.then([done](int result) {
            EXPECT_EQ(result, 12);
            done->set_value();
        });
    });
}

TYPED_TEST(AsyncTest, do_returnValueWithParametersAsRValue)
{
    run_in_executor(this->executor_, [&](auto done) {
        auto future = asyncly::async(
            this->executor_, [](int&& a, int&& b) { return a + b; }, 5, 7);
        future.then([done](int result) {
            EXPECT_EQ(result, 12);
            done->set_value();
        });
    });
}

TYPED_TEST(AsyncTest, do_returnValueWithParametersAsLRef)
{
    int i = 5;
    int j = 7;
    run_in_executor(this->executor_, [&](auto done) {
        auto future = asyncly::async(
            this->executor_, [](int& a, int& b) { return a + b; }, std::ref(i), std::ref(j));
        future.then([done](int result) {
            EXPECT_EQ(result, 12);
            done->set_value();
        });
    });
}

TYPED_TEST(AsyncTest, do_returnValueWithParametersReferencesAsValue)
{

    run_in_executor(this->executor_, [&](auto done) {
        int i = 5;
        int j = 7;
        auto future = asyncly::async(
            this->executor_, [](int a, int b) { return a + b; }, i, j);
        future.then([done](int result) {
            EXPECT_EQ(result, 12);
            done->set_value();
        });
    });
}

TYPED_TEST(AsyncTest, do_pointerToMemberFunction_returnValueWithParametersReferencesAsValue)
{

    struct foo_t {
        int add(int a, int b)
        {
            return a + b + c;
        }
        int c = 12;
    } foo;
    run_in_executor(this->executor_, [&](auto done) {
        auto future = asyncly::async(this->executor_, &foo_t::add, foo, 5, 7);
        future.then([done](int result) {
            EXPECT_EQ(result, 24);
            done->set_value();
        });
    });
}

TYPED_TEST(AsyncTest, do_rejectFutureOnException)
{
    run_in_executor(this->executor_, [&](auto done) {
        auto future = asyncly::async(this->executor_, []() {
                          throw std::runtime_error("this is a test");
                      }).catch_error([done](std::exception_ptr e) {
            try {
                std::rethrow_exception(e);
            } catch (const std::runtime_error& r) {
                EXPECT_EQ(r.what(), std::string("this is a test"));
            } catch (...) {
                EXPECT_TRUE(false);
            }
            done->set_value();
        });
    });
}

} // namespace asyncly