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

#include "asyncly/future/WhenAll.h"

#include "StrandImplTestFactory.h"
#include "asyncly/test/ExecutorTestFactories.h"

#include "boost/tuple/tuple_comparison.hpp"

#include "gmock/gmock.h"

#include <tuple>

namespace asyncly {

using namespace testing;

template <typename TExecutorFactory> class WhenAllTest : public Test {
  public:
    WhenAllTest()
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

TYPED_TEST_SUITE(WhenAllTest, ExecutorFactoryTypes);

TYPED_TEST(WhenAllTest, shouldCompileWhenAllWithRvalueFuturesOfSameType)
{
    this->executor_->post([]() {
        auto f = asyncly::when_all(
            asyncly::make_ready_future(std::string()), asyncly::make_ready_future(std::string()));
        std::ignore = f;
    });
}

TYPED_TEST(WhenAllTest, shouldCompileWhenAllWithRvalueFuturesOfMixedType)
{
    this->executor_->post([]() {
        auto f = asyncly::when_all(
            asyncly::make_ready_future(std::string()), asyncly::make_ready_future(10));
        std::ignore = f;
    });
}

TYPED_TEST(WhenAllTest, shouldResolveMultipleVoidFutures)
{
    std::promise<void> allResolved;
    this->executor_->post([&allResolved]() {
        auto lazy1 = make_lazy_future<void>();
        auto future1 = std::get<0>(lazy1);
        auto promise1 = std::get<1>(lazy1);
        auto lazy2 = make_lazy_future<void>();
        auto future2 = std::get<0>(lazy2);
        auto promise2 = std::get<1>(lazy2);

        when_all(std::move(future1), std::move(future2)).then([&allResolved]() {
            allResolved.set_value();
        });
        promise1.set_value();
        promise2.set_value();
    });

    allResolved.get_future().wait();
}

TYPED_TEST(WhenAllTest, shouldResolveMultipleValueFutures)
{
    //! [WhenAll Combination]
    std::promise<std::tuple<int, bool>> allResolved;
    this->executor_->post([&allResolved]() {
        auto lazy1 = make_lazy_future<int>();
        auto future1 = std::get<0>(lazy1);
        auto promise1 = std::get<1>(lazy1);
        auto lazy2 = make_lazy_future<bool>();
        auto future2 = std::get<0>(lazy2);
        auto promise2 = std::get<1>(lazy2);

        when_all(std::move(future1), std::move(future2)).then([&allResolved](int a, bool b) {
            allResolved.set_value(std::make_tuple(a, b));
        });

        promise1.set_value(10);
        promise2.set_value(false);
    });

    EXPECT_EQ(std::make_tuple(10, false), allResolved.get_future().get());
    //! [WhenAll Combination]
}

TYPED_TEST(WhenAllTest, shouldResolveMultipleMixedVoidValueFutures)
{
    std::promise<std::tuple<int, bool>> allResolved;
    this->executor_->post([&allResolved]() {
        auto lazy1 = make_lazy_future<int>();
        auto future1 = std::get<0>(lazy1);
        auto promise1 = std::get<1>(lazy1);
        auto lazy2 = make_lazy_future<void>();
        auto future2 = std::get<0>(lazy2);
        auto promise2 = std::get<1>(lazy2);
        auto lazy3 = make_lazy_future<bool>();
        auto future3 = std::get<0>(lazy3);
        auto promise3 = std::get<1>(lazy3);

        when_all(std::move(future1), std::move(future2), std::move(future3))
            .then([&allResolved](int a, bool b) { allResolved.set_value(std::make_tuple(a, b)); });
        promise1.set_value(10);
        promise2.set_value();
        promise3.set_value(true);
    });

    EXPECT_EQ(std::make_tuple(10, true), allResolved.get_future().get());
}

TYPED_TEST(WhenAllTest, shouldCompileWhenAllWithContinuationReturningValue)
{
    const auto a = 23;
    const auto b = 42;
    std::promise<int> sumPromise;
    this->executor_->post([&]() {
        auto aFuture = asyncly::make_ready_future<int>(a);
        auto bFuture = asyncly::make_ready_future<int>(b);
        when_all(std::move(aFuture), std::move(bFuture))
            .then([](auto a, auto b) { return a + b; })
            .then([&sumPromise](auto c) { sumPromise.set_value(c); });
    });
    EXPECT_EQ(sumPromise.get_future().get(), a + b);
}

TYPED_TEST(WhenAllTest, shouldCompileWhenAllWithContinuationReturningVoidFuture)
{
    const auto a = 23;
    const auto b = 42;
    int sum = 0;
    std::promise<void> donePromise;
    this->executor_->post([&]() {
        auto aFuture = asyncly::make_ready_future<int>(a);
        auto bFuture = asyncly::make_ready_future<int>(b);
        when_all(std::move(aFuture), std::move(bFuture))
            .then([&sum](auto a, auto b) {
                sum = a + b;
                return asyncly::make_ready_future();
            })
            .then([&donePromise]() { donePromise.set_value(); });
    });
    donePromise.get_future().get();
    EXPECT_EQ(sum, a + b);
}

TYPED_TEST(WhenAllTest, shouldCompileWhenAllWithContinuationReturningValueFuture)
{
    const auto a = 23;
    const auto b = 42;
    std::promise<int> sumPromise;
    this->executor_->post([&]() {
        auto aFuture = asyncly::make_ready_future<int>(a);
        auto bFuture = asyncly::make_ready_future<int>(b);
        when_all(std::move(aFuture), std::move(bFuture))
            .then([](auto a, auto b) { return asyncly::make_ready_future(a + b); })
            .then([&sumPromise](auto c) { sumPromise.set_value(c); });
    });
    EXPECT_EQ(sumPromise.get_future().get(), a + b);
}

TYPED_TEST(WhenAllTest, shouldRejectWhenAllOnFirstError)
{
    struct MyError : public std::exception {
    };

    std::promise<void> rejected;
    this->executor_->post([&rejected]() {
        auto lazy1 = make_lazy_future<int>();
        auto future1 = std::get<0>(lazy1);
        auto promise1 = std::get<1>(lazy1);
        auto lazy2 = make_lazy_future<void>();
        auto future2 = std::get<0>(lazy2);
        auto promise2 = std::get<1>(lazy2);
        auto lazy3 = make_lazy_future<bool>();
        auto future3 = std::get<0>(lazy3);
        auto promise3 = std::get<1>(lazy3);

        when_all(std::move(future1), std::move(future2), std::move(future3))
            .then([](int, bool) { FAIL(); })
            .catch_error([&rejected](auto error) {
                try {
                    std::rethrow_exception(error);
                } catch (...) {
                    rejected.set_exception(std::current_exception());
                }
            });
        promise1.set_exception(MyError{});
    });

    EXPECT_THROW(rejected.get_future().get(), MyError);
}

TYPED_TEST(WhenAllTest, shouldIgnoreResolveAfterReject)
{
    std::promise<void> rejected;
    auto lazy1 = make_lazy_future<void>();
    auto future1 = std::get<0>(lazy1);
    auto promise1 = std::get<1>(lazy1);
    auto lazy2 = make_lazy_future<int>();
    auto future2 = std::get<0>(lazy2);
    auto promise2 = std::get<1>(lazy2);
    this->executor_->post([&rejected, &future1, &future2]() mutable {
        when_all(std::move(future1), std::move(future2))
            .then([](auto) { FAIL(); })
            .catch_error([&rejected](auto) mutable { rejected.set_value(); });
    });
    promise1.set_exception("bla1");
    EXPECT_NO_THROW(rejected.get_future().get());
    promise2.set_value(1);
}

TYPED_TEST(WhenAllTest, shouldIgnoreRejectAfterReject)
{
    std::promise<void> rejected;
    auto lazy1 = make_lazy_future<void>();
    auto future1 = std::get<0>(lazy1);
    auto promise1 = std::get<1>(lazy1);
    auto lazy2 = make_lazy_future<int>();
    auto future2 = std::get<0>(lazy2);
    auto promise2 = std::get<1>(lazy2);
    this->executor_->post([&rejected, &future1, &future2]() mutable {
        when_all(std::move(future1), std::move(future2))
            .then([](auto) { FAIL(); })
            .catch_error([&rejected](auto) mutable { rejected.set_value(); });
    });
    promise1.set_exception("bla1");
    EXPECT_NO_THROW(rejected.get_future().get());
    promise2.set_exception("bla2");
}

TYPED_TEST(WhenAllTest, shouldResolveMultipleObjectValueFutures)
{
    auto expect1 = std::string("hello");
    auto expect2 = std::make_shared<int>(10);
    std::promise<std::tuple<decltype(expect1), decltype(expect2)>> allResolved;
    this->executor_->post([&allResolved, expect1, expect2]() {
        make_ready_future()
            .then([expect1, expect2]() {
                return when_all(
                    make_ready_future<std::string>(expect1),
                    make_ready_future<std::shared_ptr<int>>(expect2));
            })
            .then([&allResolved](std::string a, std::shared_ptr<int> b) {
                allResolved.set_value(std::make_tuple(a, b));
            });
    });

    EXPECT_EQ(std::make_tuple(expect1, expect2), allResolved.get_future().get());
}

TYPED_TEST(WhenAllTest, shouldResolveContainerOfFutures)
{
    std::vector<Future<int>> futures;
    futures.push_back(make_ready_future<int>(1));
    futures.push_back(make_ready_future<int>(2));
    futures.push_back(make_ready_future<int>(3));

    std::promise<std::vector<int>> results;
    auto resultsFuture = results.get_future();

    this->executor_->post([&futures, resultsPromise = std::move(results)]() mutable {
        when_all(futures.begin(), futures.end())
            .then([resultsPromise2 = std::move(resultsPromise)](auto result) mutable {
                resultsPromise2.set_value(result);
            });
    });
    EXPECT_THAT(resultsFuture.get(), ContainerEq(std::vector<int>{ 1, 2, 3 }));
}

TYPED_TEST(WhenAllTest, shouldResolveEmptyContainerOfFutures)
{
    std::vector<Future<int>> futures;

    std::promise<std::vector<int>> results;
    auto resultsFuture = results.get_future();

    this->executor_->post([&futures, resultsPromise = std::move(results)]() mutable {
        when_all(futures.begin(), futures.end())
            .then([resultsPromise2 = std::move(resultsPromise)](auto result) mutable {
                resultsPromise2.set_value(result);
            });
    });
    EXPECT_THAT(resultsFuture.get(), ContainerEq(std::vector<int>{}));
}

TYPED_TEST(WhenAllTest, shouldResolveContainerOfVoidFutures)
{
    std::vector<Future<void>> futures;
    futures.push_back(make_ready_future());
    futures.push_back(make_ready_future());
    futures.push_back(make_ready_future());

    std::promise<void> results;
    auto resultsFuture = results.get_future();

    this->executor_->post([&futures, resultsPromise = std::move(results)]() mutable {
        when_all(futures.begin(), futures.end())
            .then([resultsPromise2 = std::move(resultsPromise)]() mutable {
                resultsPromise2.set_value();
            });
    });
    EXPECT_NO_THROW(resultsFuture.get());
}

TYPED_TEST(WhenAllTest, shouldResolveEmptyContainerOfVoidFutures)
{
    std::vector<Future<void>> futures;

    std::promise<void> results;
    auto resultsFuture = results.get_future();

    this->executor_->post([&futures, resultsPromise = std::move(results)]() mutable {
        when_all(futures.begin(), futures.end())
            .then([resultsPromise2 = std::move(resultsPromise)]() mutable {
                resultsPromise2.set_value();
            });
    });
    EXPECT_NO_THROW(resultsFuture.get());
}

TYPED_TEST(WhenAllTest, shouldRejectContainerOfFutures)
{
    struct CustomError : public std::exception {
    };

    std::vector<Future<int>> futures;
    futures.push_back(make_ready_future<int>(3));
    futures.push_back(make_exceptional_future<int>(CustomError{}));
    futures.push_back(make_ready_future<int>(5));

    std::promise<void> results;
    auto resultsFuture = results.get_future();

    this->executor_->post([&futures, resultsPromise = std::move(results)]() mutable {
        when_all(futures.begin(), futures.end())
            .then([](auto) mutable { FAIL(); })
            .catch_error([resultsPromise2 = std::move(resultsPromise)](auto e) mutable {
                resultsPromise2.set_exception(e);
            });
    });
    EXPECT_THROW(resultsFuture.get(), CustomError);
}

TYPED_TEST(WhenAllTest, shouldRejectContainerOfVoidFutures)
{
    struct CustomError : public std::exception {
    };

    std::vector<Future<void>> futures;
    futures.push_back(make_ready_future());
    futures.push_back(make_exceptional_future<void>(CustomError{}));
    futures.push_back(make_ready_future());

    std::promise<void> results;
    auto resultsFuture = results.get_future();

    this->executor_->post([&futures, resultsPromise = std::move(results)]() mutable {
        when_all(futures.begin(), futures.end())
            .then([]() mutable { FAIL(); })
            .catch_error([resultsPromise2 = std::move(resultsPromise)](auto e) mutable {
                resultsPromise2.set_exception(e);
            });
    });
    EXPECT_THROW(resultsFuture.get(), CustomError);
}

TYPED_TEST(WhenAllTest, shouldRejectContainerOfVoidFuturesWithMultipleErrors)
{
    struct CustomError : public std::exception {
    };

    std::vector<Future<void>> futures;
    futures.push_back(make_ready_future());
    futures.push_back(make_exceptional_future<void>(CustomError{}));
    futures.push_back(make_exceptional_future<void>(CustomError{}));

    std::promise<void> results;
    auto resultsFuture = results.get_future();

    this->executor_->post([&futures, resultsPromise = std::move(results)]() mutable {
        when_all(futures.begin(), futures.end())
            .then([]() mutable { FAIL(); })
            .catch_error([resultsPromise2 = std::move(resultsPromise)](auto e) mutable {
                resultsPromise2.set_exception(e);
            });
    });
    EXPECT_THROW(resultsFuture.get(), CustomError);
}
} // namespace asyncly
