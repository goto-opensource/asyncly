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

#include "asyncly/future/WhenAny.h"

#include "StrandImplTestFactory.h"
#include "asyncly/future/Future.h"
#include "asyncly/test/ExecutorTestFactories.h"

#include "boost/none_t.hpp"

#include "gmock/gmock.h"

namespace asyncly {

using namespace testing;

template <typename TExecutorFactory> class WhenAnyTest : public Test {
  public:
    WhenAnyTest()
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

TYPED_TEST_SUITE(WhenAnyTest, ExecutorFactoryTypes);

TYPED_TEST(WhenAnyTest, shouldResolveValueFutures)
{
    std::promise<int> resolved;

    this->executor_->post([&resolved]() {
        auto lazy1 = make_lazy_future<int>();
        auto future1 = std::get<0>(lazy1);
        auto promise1 = std::get<1>(lazy1);
        auto lazy2 = make_lazy_future<bool>();
        auto future2 = std::get<0>(lazy2);
        auto promise2 = std::get<1>(lazy2);

        when_any(future1, future2).then([&resolved](boost::variant2::variant<int, bool> result) {
            // in C++17 this could be written much more concisely, see
            // http://en.cppreference.com/w/cpp/utility/variant/visit
            struct visitor {
                visitor(std::promise<int>& resolved)
                    : resolved_{ resolved }
                {
                }

                void operator()(int a) const
                {
                    resolved_.set_value(a);
                }

                void operator()(bool) const
                {
                }

                std::promise<int>& resolved_;
            };

            boost::variant2::visit(visitor{ resolved }, result);
        });

        promise1.set_value(5);
        promise2.set_value(true);
    });

    EXPECT_EQ(5, resolved.get_future().get());
}

TYPED_TEST(WhenAnyTest, shouldResolveMixedVoidValueFutures)
{
    std::promise<void> resolved;

    this->executor_->post([&resolved]() {
        auto lazy1 = make_lazy_future<int>();
        auto future1 = std::get<0>(lazy1);
        auto promise1 = std::get<1>(lazy1);
        auto lazy2 = make_lazy_future<void>();
        auto future2 = std::get<0>(lazy2);
        auto promise2 = std::get<1>(lazy2);

        when_any(future1, future2)
            .then([&resolved](boost::variant2::variant<int, boost::none_t> result) {
                // in C++17 this could be written much more concisely, see
                // http://en.cppreference.com/w/cpp/utility/variant/visit
                struct visitor {
                    visitor(std::promise<void>& resolved)
                        : resolved_{ resolved }
                    {
                    }

                    void operator()(int) const
                    {
                    }

                    void operator()(boost::none_t) const
                    {
                        resolved_.set_value();
                    }

                    std::promise<void>& resolved_;
                };

                boost::variant2::visit(visitor{ resolved }, result);
            });

        promise2.set_value();
        promise1.set_value(5);
    });

    EXPECT_NO_THROW(resolved.get_future().get());
}

TYPED_TEST(WhenAnyTest, shouldResolveMixedVoidValueFuturesWhenVoidPromiseIsNotResolvedFirst)
{
    std::promise<int> resolved;

    this->executor_->post([&resolved]() {
        auto lazy1 = make_lazy_future<int>();
        auto future1 = std::get<0>(lazy1);
        auto promise1 = std::get<1>(lazy1);
        auto lazy2 = make_lazy_future<void>();
        auto future2 = std::get<0>(lazy2);
        auto promise2 = std::get<1>(lazy2);

        when_any(future1, future2)
            .then([&resolved](boost::variant2::variant<int, boost::none_t> result) {
                // in C++17 this could be written much more concisely, see
                // http://en.cppreference.com/w/cpp/utility/variant/visit
                struct visitor {
                    visitor(std::promise<int>& resolved)
                        : resolved_{ resolved }
                    {
                    }

                    void operator()(int a) const
                    {
                        resolved_.set_value(a);
                    }

                    void operator()(boost::none_t) const
                    {
                    }

                    std::promise<int>& resolved_;
                };

                boost::variant2::visit(visitor{ resolved }, result);
            });

        promise1.set_value(5);
        promise2.set_value();
    });

    EXPECT_EQ(5, resolved.get_future().get());
}

TYPED_TEST(WhenAnyTest, shouldResolveValueFutureContinuations)
{
    //! [WhenAny Combination]
    std::promise<double> resolved;

    this->executor_->post([&resolved]() {
        auto lazy1 = make_lazy_future<int>();
        auto future1 = std::get<0>(lazy1);
        auto promise1 = std::get<1>(lazy1);
        auto lazy2 = make_lazy_future<double>();
        auto future2 = std::get<0>(lazy2);
        auto promise2 = std::get<1>(lazy2);

        when_any(future1, future2)
            .then([](boost::variant2::variant<int, double> result) {
                struct visitor {
                    double operator()(int a) const
                    {
                        return static_cast<double>(a);
                    }

                    double operator()(double b) const
                    {
                        return b;
                    }
                };

                return make_ready_future<double>(boost::variant2::visit(visitor(), result));
            })
            .then([&resolved](double result) { resolved.set_value(result); });

        promise1.set_value(5);
        promise2.set_value(10.0);
    });

    EXPECT_DOUBLE_EQ(5.0, resolved.get_future().get());
    //! [WhenAny Combination]
}

TYPED_TEST(WhenAnyTest, shouldSupportErrors)
{
    std::promise<double> resolved;

    struct MyError : public std::exception { };

    this->executor_->post([&resolved]() {
        auto lazy1 = make_lazy_future<int>();
        auto future1 = std::get<0>(lazy1);
        auto promise1 = std::get<1>(lazy1);
        auto lazy2 = make_lazy_future<double>();
        auto future2 = std::get<0>(lazy2);
        auto promise2 = std::get<1>(lazy2);

        when_any(future1, future2)
            .then([](boost::variant2::variant<int, double>) {})
            .catch_error([&resolved](auto error) { resolved.set_exception(error); });

        promise1.set_exception(MyError{});
    });

    EXPECT_THROW(resolved.get_future().get(), MyError);
}

TYPED_TEST(WhenAnyTest, shouldIgnoreResolveAfterRejectForWhenAny)
{
    std::promise<void> rejected;
    auto lazy1 = make_lazy_future<void>();
    auto future1 = std::get<0>(lazy1);
    auto promise1 = std::get<1>(lazy1);
    auto lazy2 = make_lazy_future<int>();
    auto future2 = std::get<0>(lazy2);
    auto promise2 = std::get<1>(lazy2);
    this->executor_->post([&rejected, &future1, &future2]() mutable {
        when_any(std::move(future1), std::move(future2))
            .then([](auto) { FAIL(); })
            .catch_error([&rejected](auto) mutable { rejected.set_value(); });
    });
    promise1.set_exception("bla1");
    EXPECT_NO_THROW(rejected.get_future().get());
    promise2.set_value(1);
}
} // namespace asyncly
