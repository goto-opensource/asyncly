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

#include <chrono>
#include <future>
#include <thread>

#include "gmock/gmock.h"

#include "asyncly/future/Future.h"

#include "asyncly/test/ExecutorTestFactories.h"
#include "detail/ThrowingExecutor.h"

namespace asyncly {

using namespace testing;

template <typename TExecutorFactory> class FutureTest : public Test {
  public:
    FutureTest()
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

TYPED_TEST_SUITE(FutureTest, ExecutorFactoryTypes);

TYPED_TEST(FutureTest, shouldRunASimpleContinuation)
{
    std::promise<int> value;

    this->executor_->post([&value]() {
        make_ready_future(42).then([&value](int v) {
            value.set_value(v);
            return make_ready_future();
        });
    });

    EXPECT_EQ(42, value.get_future().get());
}

TYPED_TEST(FutureTest, shouldRunLValueContinuation)
{
    std::promise<int> value;

    const auto lambda = [&value](int v) {
        value.set_value(v);
        return make_ready_future();
    };

    this->executor_->post([&lambda]() { make_ready_future(42).then(lambda); });

    EXPECT_EQ(42, value.get_future().get());
}

TYPED_TEST(FutureTest, shouldRunANoncopyableContinuation)
{
    std::promise<int> value;
    auto future = value.get_future();

    this->executor_->post([&value]() {
        make_ready_future(42).then([promise{ std::move(value) }](int v) mutable {
            promise.set_value(v);
            return make_ready_future();
        });
    });

    EXPECT_EQ(42, future.get());
}

TYPED_TEST(FutureTest, shouldRunANoncopyableContinuationReturningVoid)
{
    std::promise<int> value;
    auto future = value.get_future();

    this->executor_->post([&value]() {
        make_ready_future(42).then(
            [promise{ std::move(value) }](const int& v) mutable { promise.set_value(v); });
    });

    EXPECT_EQ(42, future.get());
}

TYPED_TEST(FutureTest, shouldRunAChainedContinuationWithFutures)
{
    std::promise<int> value1;
    std::promise<float> value2;

    this->executor_->post([&value1, &value2]() {
        make_ready_future(42)
            .then([&value1](int v) {
                value1.set_value(v);
                return make_ready_future(v / 2.0f);
            })
            .then([&value2](float v) { value2.set_value(v); });
    });

    EXPECT_EQ(42, value1.get_future().get());
    EXPECT_FLOAT_EQ(21.0, value2.get_future().get());
}

TYPED_TEST(FutureTest, shouldRunAChainedContinuationWithValues)
{
    //! [Future Chain]
    std::promise<int> value1;
    std::promise<float> value2;

    this->executor_->post([&value1, &value2]() {
        make_ready_future(42)
            .then([&value1](int v) {
                value1.set_value(v);
                return v / 2.0f;
            })
            .then([&value2](float v) { value2.set_value(v); });
    });

    EXPECT_EQ(42, value1.get_future().get());
    EXPECT_FLOAT_EQ(21.0, value2.get_future().get());
    //! [Future Chain]
}

TYPED_TEST(FutureTest, shouldWorkWithLazyFutures)
{
    //! [Lazy Future]
    std::promise<void> called;
    this->executor_->post([&called]() {
        auto lazy = make_lazy_future<void>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.then([&called]() { called.set_value(); });

        promise.set_value();
    });

    called.get_future().wait();
    //! [Lazy Future]
}

TYPED_TEST(FutureTest, shouldCreateReadyFuturesFromLValues)
{
    std::promise<int> called;
    this->executor_->post([&called]() {
        auto v = 1;
        make_ready_future(v).then([&called](int value) { called.set_value(value); });
    });

    EXPECT_EQ(1, called.get_future().get());
}

TYPED_TEST(FutureTest, shouldCreateReadyFuturesFromRValues)
{
    std::promise<int> called;
    this->executor_->post([&called]() {
        auto v = 1;
        make_ready_future(std::move(v)).then([&called](int value) { called.set_value(value); });
    });

    EXPECT_EQ(1, called.get_future().get());
}

TYPED_TEST(FutureTest, shouldResolvePromisesFromRValues)
{
    std::promise<bool> called;
    this->executor_->post([&called]() {
        auto value = true;

        auto lazy = make_lazy_future<bool>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.then([&called](bool v) { called.set_value(v); });

        promise.set_value(std::move(value));
    });

    EXPECT_EQ(true, called.get_future().get());
}

TYPED_TEST(FutureTest, shouldResolvePromisesFromLValues)
{
    std::promise<bool> called;
    this->executor_->post([&called]() {
        auto value = true;

        auto lazy = make_lazy_future<bool>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.then([&called](bool v) { called.set_value(v); });

        promise.set_value(value);
    });

    EXPECT_EQ(true, called.get_future().get());
}

TYPED_TEST(FutureTest, shouldCallThenInTheRightExecutorContext)
{
    std::promise<std::thread::id> executorThreadIdPromise;
    this->executor_->post([&executorThreadIdPromise]() {
        executorThreadIdPromise.set_value(std::this_thread::get_id());
    });

    auto executorThreadId = executorThreadIdPromise.get_future().get();

    auto lazy = make_lazy_future<void>();
    auto future = std::get<0>(lazy);
    auto promise = std::get<1>(lazy);

    std::promise<std::thread::id> continuationThreadIdPromise;

    this->executor_->post([&future, &continuationThreadIdPromise]() {
        future.then([&continuationThreadIdPromise]() {
            continuationThreadIdPromise.set_value(std::this_thread::get_id());
        });
    });

    promise.set_value();
    EXPECT_EQ(executorThreadId, continuationThreadIdPromise.get_future().get());
}

TYPED_TEST(FutureTest, shouldCatchExceptionalFuture)
{
    //! [Make Exceptional Future]
    std::promise<void> exception;
    this->executor_->post([&exception]() {
        std::exception_ptr e;
        try {
            throw std::runtime_error("test error");
        } catch (...) {
            e = std::current_exception();
        }
        auto future = make_exceptional_future<int>(e);
        future.catch_error([&exception](std::exception_ptr ex) { exception.set_exception(ex); });
    });
    EXPECT_THROW(exception.get_future().get(), std::runtime_error);
    //! [Make Exceptional Future]
}

TYPED_TEST(FutureTest, shouldCatchErrorsWithoutAChain)
{
    std::promise<void> exception;
    this->executor_->post([&exception]() {
        auto lazy = make_lazy_future<void>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.then([]() { throw std::runtime_error("intentional exception"); })
            .catch_error([&exception](std::exception_ptr e) { exception.set_exception(e); });

        promise.set_value();
    });

    EXPECT_THROW(exception.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldCatchErrorsWithoutAChainAndNonCopyableHandler)
{
    std::promise<void> exception;
    auto future = exception.get_future();
    this->executor_->post([exception{ std::move(exception) }]() mutable {
        auto lazy = make_lazy_future<void>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.then([]() { throw std::runtime_error("intentional exception"); })
            .catch_error([exception{ std::move(exception) }](std::exception_ptr e) mutable {
                exception.set_exception(e);
            });

        promise.set_value();
    });

    EXPECT_THROW(future.get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldCatchErrorsInAChain)
{
    std::promise<void> exception;
    bool shouldNeverBeTrue = false;
    this->executor_->post([&exception, &shouldNeverBeTrue]() {
        auto lazy = make_lazy_future<void>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.then([]() -> Future<void> { throw std::runtime_error("intentional exception"); })
            .then([&shouldNeverBeTrue]() { shouldNeverBeTrue = true; })
            .catch_error([&exception](std::exception_ptr e) { exception.set_exception(e); });

        promise.set_value();
    });

    EXPECT_THROW(exception.get_future().get(), std::runtime_error);
    EXPECT_FALSE(shouldNeverBeTrue);
}

TYPED_TEST(FutureTest, shouldMakeExceptionalFutureFromStdException)
{
    std::promise<void> exception;
    this->executor_->post([&exception]() {
        make_exceptional_future<void>(std::logic_error{ "intentional exception" })
            .catch_error([&exception](std::exception_ptr e) { exception.set_exception(e); });
    });

    EXPECT_THROW(exception.get_future().get(), std::logic_error);
}

TYPED_TEST(FutureTest, shouldMakeExceptionalFutureFromStdString)
{
    std::promise<void> exception;
    this->executor_->post([&exception]() {
        make_exceptional_future<void>(std::string{ "intentional exception" })
            .catch_error([&exception](std::exception_ptr e) { exception.set_exception(e); });
    });

    EXPECT_THROW(exception.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldMakeExceptionalFutureFromZeroTerminatedString)
{
    std::promise<void> exception;
    this->executor_->post([&exception]() {
        make_exceptional_future<void>("intentional exception")
            .catch_error([&exception](std::exception_ptr e) { exception.set_exception(e); });
    });

    EXPECT_THROW(exception.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldAllowForLazyExecptionsFromStdExceptionPtr)
{
    std::promise<void> exception;

    this->executor_->post([&exception]() {
        auto lazy = make_lazy_future<void>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.catch_error([&exception](std::exception_ptr e) { exception.set_exception(e); });

        promise.set_exception(std::make_exception_ptr(std::logic_error{ "logic_error" }));
    });

    EXPECT_THROW(exception.get_future().get(), std::logic_error);
}

TYPED_TEST(FutureTest, shouldAllowForLazyExecptionsFromStdException)
{
    std::promise<void> exception;

    this->executor_->post([&exception]() {
        auto lazy = make_lazy_future<void>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.catch_error([&exception](std::exception_ptr e) { exception.set_exception(e); });

        promise.set_exception(std::logic_error{ "logic_error" });
    });

    EXPECT_THROW(exception.get_future().get(), std::logic_error);
}

TYPED_TEST(FutureTest, shouldAllowForLazyExecptionsFromStdString)
{
    std::promise<void> exception;

    this->executor_->post([&exception]() {
        auto lazy = make_lazy_future<void>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.catch_error([&exception](std::exception_ptr e) { exception.set_exception(e); });

        promise.set_exception(std::string{ "logic_error" });
    });

    EXPECT_THROW(exception.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldAllowForLazyExecptionsFromCharPtr)
{
    std::promise<void> exception;

    this->executor_->post([&exception]() {
        auto lazy = make_lazy_future<void>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.catch_error([&exception](std::exception_ptr e) { exception.set_exception(e); });

        promise.set_exception("logic_error");
    });

    EXPECT_THROW(exception.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldHandleLazyUserErrors)
{
    std::promise<void> exception;
    this->executor_->post([&exception]() {
        std::exception_ptr e;
        try {
            throw std::runtime_error("test error");
        } catch (...) {
            e = std::current_exception();
        }
        make_ready_future()
            .then([e]() { return make_exceptional_future<int>(e); })
            .catch_error([&exception](std::exception_ptr ex) { exception.set_exception(ex); });
    });
    EXPECT_THROW(exception.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldForwardValuesThroughCatch)
{
    std::promise<int> propagated_value;
    this->executor_->post([&propagated_value]() {
        make_ready_future()
            .then([]() { return 5; })
            .catch_error([](std::exception_ptr) {})
            .then([&propagated_value](int value) { propagated_value.set_value(value); });
    });
    EXPECT_EQ(5, propagated_value.get_future().get());
}

TYPED_TEST(FutureTest, shouldChooseTheRightErrorHandler)
{
    //! [Error Chain]
    struct WrongException : std::exception {
    };
    struct CorrectException : std::exception {
    };
    std::promise<void> exception_container;

    this->executor_->post([&exception_container]() {
        make_ready_future()
            .then([]() { return 5; })
            .catch_error([&exception_container](std::exception_ptr) {
                std::exception_ptr e;
                try {
                    throw WrongException{};
                } catch (...) {
                    e = std::current_exception();
                }
                exception_container.set_exception(e);
            })
            .then([](int) -> Future<int> { throw CorrectException{}; })
            .catch_error([&exception_container](std::exception_ptr e) {
                exception_container.set_exception(e);
            })
            .then([](int value) { return value; })
            .catch_error([&exception_container](std::exception_ptr) {
                std::exception_ptr e;
                try {
                    throw WrongException{};
                } catch (...) {
                    e = std::current_exception();
                }
                exception_container.set_exception(e);
            });
    });
    EXPECT_THROW(exception_container.get_future().get(), CorrectException);
    //! [Error Chain]
}

TYPED_TEST(FutureTest, shouldChooseTheRightErrorHandlerOnExceptionalFuture)
{
    struct WrongException : std::exception {
    };
    struct CorrectException : std::exception {
    };
    std::promise<void> exception_container;

    this->executor_->post([&exception_container]() {
        make_exceptional_future<int>(CorrectException{})
            .then([](int i) { return i; })
            .catch_error([&exception_container](std::exception_ptr e) {
                exception_container.set_exception(e);
            })
            .then([](int value) { return value; })
            .catch_error([&exception_container](std::exception_ptr) {
                std::exception_ptr e;
                try {
                    throw WrongException{};
                } catch (...) {
                    e = std::current_exception();
                }
                exception_container.set_exception(e);
            });
    });
    EXPECT_THROW(exception_container.get_future().get(), CorrectException);
}

TYPED_TEST(FutureTest, shouldCatchAndForwardError)
{
    struct CorrectException : std::exception {
    };
    std::promise<void> exception1_container;
    std::promise<void> exception2_container;

    this->executor_->post([&exception1_container, &exception2_container]() {
        // Use case: handle error internally within a function but also externally on the caller
        // side.
        // Setup:
        // `auto foo() {return future.catch_and_forward_error(internal_error_handler);}`
        // `foo().then(continuation).catch_error(external_error_handler);`
        // Both error handlers (`internal_error_handler` and `external_error_handler`) should get
        // called.
        make_exceptional_future<int>(CorrectException{})
            .then([](int i) { return i; })
            .catch_and_forward_error([&exception1_container](std::exception_ptr e) {
                exception1_container.set_exception(e);
            })
            .then([](int value) { return value; })
            .catch_error([&exception2_container](std::exception_ptr e) {
                exception2_container.set_exception(e);
            });
    });
    EXPECT_THROW(exception1_container.get_future().get(), CorrectException);
    EXPECT_THROW(exception2_container.get_future().get(), CorrectException);
}

TYPED_TEST(FutureTest, shouldWorkWithFutureProxies)
{
    auto callerExecutorControl = ThreadPoolExecutorController::create(1);
    auto callerExecutor = callerExecutorControl->get_executor();

    auto calleeExecutorControl = ThreadPoolExecutorController::create(1);
    auto calleeExecutor = calleeExecutorControl->get_executor();

    struct Interface {
        virtual ~Interface() = default;
        virtual Future<int> op() = 0;
    };

    struct Implementation : public Interface {
        Future<int> op() override
        {
            return make_ready_future(42);
        }
    };

    struct Proxy : public Interface {
        Proxy(
            const std::shared_ptr<IExecutor>& executor,
            const std::shared_ptr<Interface>& implementation)
            : executor_(executor)
            , implementation_(implementation)
        {
        }

        Future<int> op() override
        {
            auto lazy = make_lazy_future<int>();
            auto future = std::get<0>(lazy);
            auto promise = std::get<1>(lazy);

            auto callerExecutor = this_thread::get_current_executor();
            auto p = implementation_;
            executor_->post([p, promise, callerExecutor]() mutable {
                p->op().then([callerExecutor, promise](int value) mutable {
                    callerExecutor->post(
                        [promise, value]() mutable { promise.set_value(std::move(value)); });
                });
            });

            return future;
        }

        const std::shared_ptr<IExecutor> executor_;
        const std::shared_ptr<Interface> implementation_;
    };

    auto implementation = std::make_shared<Implementation>();
    auto proxy = std::make_shared<Proxy>(calleeExecutor, implementation);

    std::promise<int> valuePromise;
    callerExecutor->post([proxy, &valuePromise]() {
        proxy->op().then([&valuePromise](int value) { valuePromise.set_value(value); });
    });

    EXPECT_EQ(42, valuePromise.get_future().get());
}

TYPED_TEST(FutureTest, shouldThrowWhenOverwritingValueContinuations)
{
    std::promise<void> exceptionThrown;

    this->executor_->post([&exceptionThrown]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.then([](int) {});
        try {
            future.then([](int) {});
        } catch (...) {
            exceptionThrown.set_exception(std::current_exception());
        }
    });

    ASSERT_THROW(exceptionThrown.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldThrowWhenOverwritingValueContinuationsEvenWhenResolved)
{
    std::promise<void> exceptionThrown;

    this->executor_->post([&exceptionThrown]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.then([](int) {});
        promise.set_value(42);
        try {
            future.then([](int) {});
        } catch (...) {
            exceptionThrown.set_exception(std::current_exception());
        }
    });

    ASSERT_THROW(exceptionThrown.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldThrowWhenOverwritingErrorContinuations)
{
    std::promise<void> exceptionThrown;

    this->executor_->post([&exceptionThrown]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.catch_error([](std::exception_ptr) {});
        try {
            future.catch_error([](std::exception_ptr) {});
        } catch (...) {
            exceptionThrown.set_exception(std::current_exception());
        }
    });

    ASSERT_THROW(exceptionThrown.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldThrowWhenOverwritingErrorContinuationsEvenWhenRejected)
{
    std::promise<void> exceptionThrown;

    this->executor_->post([&exceptionThrown]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.catch_error([](std::exception_ptr) {});
        promise.set_exception("intentional error");
        try {
            future.catch_error([](std::exception_ptr) {});
        } catch (...) {
            exceptionThrown.set_exception(std::current_exception());
        }
    });

    ASSERT_THROW(exceptionThrown.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldPropagateMoveOnlyValues)
{
    std::promise<std::unique_ptr<int>> propagatedValue;

    this->executor_->post([&propagatedValue]() {
        auto contained = new int(42);
        auto value = std::unique_ptr<int>{ contained };

        make_ready_future<std::unique_ptr<int>>(std::move(value))
            .then([&propagatedValue](std::unique_ptr<int> v) {
                propagatedValue.set_value(std::move(v));
            });
    });

    EXPECT_EQ(42, *propagatedValue.get_future().get());
}

TYPED_TEST(FutureTest, shouldThrowWhenSettingValueTwice)
{
    std::promise<void> shouldThrow;

    this->executor_->post([&shouldThrow]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        promise.set_value(5);
        try {
            promise.set_value(5);
        } catch (...) {
            shouldThrow.set_exception(std::current_exception());
        }
    });

    EXPECT_THROW(shouldThrow.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldThrowWhenSettingErrorsTwice)
{
    std::promise<void> shouldThrow;

    this->executor_->post([&shouldThrow]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        std::exception_ptr e;
        try {
            throw std::runtime_error("some error");
        } catch (...) {
            e = std::current_exception();
        }

        promise.set_exception(e);
        try {
            promise.set_exception(e);
        } catch (...) {
            shouldThrow.set_exception(std::current_exception());
        }
    });

    EXPECT_THROW(shouldThrow.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldThrowWhenSettingErrorAfterValue)
{
    std::promise<void> shouldThrow;

    this->executor_->post([&shouldThrow]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        std::exception_ptr e;
        try {
            throw std::runtime_error("some error");
        } catch (...) {
            e = std::current_exception();
        }

        promise.set_value(5);
        try {
            promise.set_exception(e);
        } catch (...) {
            shouldThrow.set_exception(std::current_exception());
        }
    });

    EXPECT_THROW(shouldThrow.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldThrowWhenSettingValueAfterError)
{
    std::promise<void> shouldThrow;

    this->executor_->post([&shouldThrow]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        std::exception_ptr e;
        try {
            throw std::runtime_error("some error");
        } catch (...) {
            e = std::current_exception();
        }

        promise.set_exception(e);
        try {
            promise.set_value(5);
        } catch (...) {
            shouldThrow.set_exception(std::current_exception());
        }
    });

    EXPECT_THROW(shouldThrow.get_future().get(), std::runtime_error);
}

TYPED_TEST(FutureTest, shouldDeleteTheContinuationWhenTheErrorHandlerHasBeenExecuted)
{
    std::promise<void> result;

    auto i = std::make_shared<int>(42);
    EXPECT_EQ(1, i.use_count());

    auto lazy = make_lazy_future<void>();
    auto future = std::get<0>(lazy);
    auto promise = std::get<1>(lazy);

    this->executor_->post([&future, &promise, &i, &result]() {
        future.catch_error([&result](std::exception_ptr ex) mutable { result.set_exception(ex); })
            .then([i]() {});
        EXPECT_EQ(2, i.use_count());
        promise.set_exception("failure");
    });

    EXPECT_THROW(result.get_future().get(), std::runtime_error);
    EXPECT_EQ(1, i.use_count());
}

TYPED_TEST(FutureTest, shouldDeleteTheErrorHandlerWhenTheContinuationHasBeenExecuted)
{
    std::promise<void> result;

    auto i = std::make_shared<int>(42);
    EXPECT_EQ(1, i.use_count());

    auto lazy = make_lazy_future<void>();
    auto future = std::get<0>(lazy);
    auto promise = std::get<1>(lazy);

    this->executor_->post([&future, &promise, &i, &result]() {
        future.catch_error([i](auto) {}).then([&result]() mutable { result.set_value(); });
        EXPECT_EQ(2, i.use_count());
        promise.set_value();
    });

    result.get_future().get();
    EXPECT_EQ(1, i.use_count());
}

TYPED_TEST(FutureTest, shouldDeleteTheContinuationWhenTheChainedErrorHandlerHasBeenExecuted)
{
    std::promise<void> result;

    auto i = std::make_shared<int>(42);
    EXPECT_EQ(1, i.use_count());

    auto lazy = make_lazy_future<void>();
    auto future = std::get<0>(lazy);
    auto promise = std::get<1>(lazy);

    this->executor_->post([&future, &promise, &i, &result]() {
        future.then([i]() {}).catch_error(
            [&result](std::exception_ptr ex) mutable { result.set_exception(ex); });
        EXPECT_EQ(2, i.use_count());
        promise.set_exception("failure");
    });

    EXPECT_THROW(result.get_future().get(), std::runtime_error);
    EXPECT_EQ(1, i.use_count());
}

TYPED_TEST(FutureTest, shouldDeleteTheContinuationWhenThereIsNoErrorHandlerButTheFutureIsRejected)
{
    std::promise<void> result;

    auto i = std::make_shared<int>(42);
    EXPECT_EQ(1, i.use_count());

    auto lazy = make_lazy_future<void>();
    auto future = std::get<0>(lazy);
    auto promise = std::get<1>(lazy);

    this->executor_->post([&future, &promise, &i, &result]() {
        future.then([i]() {});
        EXPECT_EQ(2, i.use_count());
        promise.set_exception("failure");
        EXPECT_EQ(1, i.use_count());
        result.set_value();
    });

    result.get_future().get();
    EXPECT_EQ(1, i.use_count());
}

TYPED_TEST(FutureTest, shouldDeleteTheErrorHandlerWhenThereIsNoContinuationButTheFutureIsResolved)
{
    std::promise<void> result;

    auto i = std::make_shared<int>(42);
    EXPECT_EQ(1, i.use_count());

    auto lazy = make_lazy_future<void>();
    auto future = std::get<0>(lazy);
    auto promise = std::get<1>(lazy);

    this->executor_->post([&future, &promise, &i, &result]() {
        future.catch_error([i](auto) {});
        EXPECT_EQ(2, i.use_count());
        promise.set_value();
        EXPECT_EQ(1, i.use_count());
        result.set_value();
    });

    result.get_future().get();
}

TYPED_TEST(FutureTest, shouldThrowWhenSchedulingContinuationsFromNonExecutorContext)
{
    auto future = make_ready_future<int>(42);

    EXPECT_THROW(future.then([](int) {}), std::runtime_error);
}

// Future<T>::then(T -> Future<void>) -> Future<void>
TYPED_TEST(FutureTest, shouldSupport_ValueFuture_ValueContinuations_ReturningVoidFutures)
{
    std::promise<void> called;

    this->executor_->post([&called]() {
        make_ready_future<int>(42).then([&called](int) {
            called.set_value();
            return make_ready_future();
        });
    });

    EXPECT_NO_THROW(called.get_future().get());
}

TYPED_TEST(
    FutureTest,
    shouldSupport_ValueFuture_ValueContinuations_ReturningVoidFutures_WithContinuationErrors)
{
    std::promise<void> called;

    struct test_error : public std::exception {
    };
    this->executor_->post([&called]() {
        make_ready_future<int>(42)
            .then([](int) -> Future<void> { throw test_error{}; })
            .catch_error([&called](std::exception_ptr e) { called.set_exception(e); });
    });

    EXPECT_THROW(called.get_future().get(), test_error);
}

TYPED_TEST(
    FutureTest,
    shouldSupport_ValueFuture_ValueContinuations_ReturningVoidFutures_WithExceptionalFutures)
{
    std::promise<void> called;

    struct test_error : public std::exception {
    };
    this->executor_->post([&called]() {
        make_ready_future<int>(42)
            .then([](int) {
                std::exception_ptr e;
                try {
                    throw test_error{};
                } catch (...) {
                    e = std::current_exception();
                }
                return make_exceptional_future<void>(e);
            })
            .catch_error([&called](std::exception_ptr e) { called.set_exception(e); });
    });

    EXPECT_THROW(called.get_future().get(), test_error);
}

// Future<void>::then(void -> Future<void>) -> Future<void>
TYPED_TEST(FutureTest, shouldSupport_VoidFuture_ValueContinuations_ReturningVoidFutures)
{
    std::promise<void> called;

    this->executor_->post([&called]() {
        make_ready_future().then([&called]() {
            called.set_value();
            return make_ready_future();
        });
    });

    EXPECT_NO_THROW(called.get_future().get());
}

TYPED_TEST(
    FutureTest,
    shouldSupport_VoidFuture_ValueContinuations_ReturningVoidFutures_WithContinuationErrors)
{
    std::promise<void> called;

    struct test_error : public std::exception {
    };
    this->executor_->post([&called]() {
        make_ready_future()
            .then([]() -> Future<void> { throw test_error{}; })
            .catch_error([&called](std::exception_ptr e) { called.set_exception(e); });
    });

    EXPECT_THROW(called.get_future().get(), test_error);
}

TYPED_TEST(
    FutureTest,
    shouldSupport_VoidFuture_ValueContinuations_ReturningVoidFutures_WithExceptionalFutures)
{
    std::promise<void> called;

    struct test_error : public std::exception {
    };
    this->executor_->post([&called]() {
        make_ready_future()
            .then([]() {
                std::exception_ptr e;
                try {
                    throw test_error{};
                } catch (...) {
                    e = std::current_exception();
                }
                return make_exceptional_future<void>(e);
            })
            .catch_error([&called](std::exception_ptr e) { called.set_exception(e); });
    });

    EXPECT_THROW(called.get_future().get(), test_error);
}

// Future<T>::then(T -> Future<U>) -> Future<U>
TYPED_TEST(FutureTest, shouldSupport_ValueFuture_ValueContinuations_ReturningValueFutures)
{
    std::promise<bool> called;

    this->executor_->post([&called]() {
        make_ready_future<int>(42)
            .then([](int) { return make_ready_future(true); })
            .then([&called](bool value) { called.set_value(value); });
    });

    EXPECT_EQ(true, called.get_future().get());
}

TYPED_TEST(
    FutureTest,
    shouldSupport_ValueFuture_ValueContinuations_ReturningValueFutures_WithContinuationErrors)
{
    std::promise<bool> called;

    struct test_error : public std::exception {
    };
    this->executor_->post([&called]() {
        make_ready_future<int>(42)
            .then([](int) -> Future<bool> { throw test_error{}; })
            .catch_error([&called](std::exception_ptr e) { called.set_exception(e); })
            .then([&called](bool value) { called.set_value(value); });
    });

    EXPECT_THROW(called.get_future().get(), test_error);
}

TYPED_TEST(
    FutureTest,
    shouldSupport_ValueFuture_ValueContinuations_ReturningValueFutures_WithExceptionalFutures)
{
    std::promise<bool> called;

    struct test_error : public std::exception {
    };
    this->executor_->post([&called]() {
        make_ready_future<int>(42)
            .then([](int) {
                std::exception_ptr e;
                try {
                    throw test_error{};
                } catch (...) {
                    e = std::current_exception();
                }
                return make_exceptional_future<bool>(e);
            })
            .catch_error([&called](std::exception_ptr e) { called.set_exception(e); })
            .then([&called](bool value) { called.set_value(value); });
    });

    EXPECT_THROW(called.get_future().get(), test_error);
}

// Future<void>::then(void -> Future<U>) -> Future<U>
TYPED_TEST(FutureTest, shouldSupport_VoidFuture_ValueContinuations_ReturningValueFutures)
{
    std::promise<bool> called;

    this->executor_->post([&called]() {
        make_ready_future()
            .then([]() { return make_ready_future(true); })
            .then([&called](bool value) { called.set_value(value); });
    });

    EXPECT_EQ(true, called.get_future().get());
}

TYPED_TEST(
    FutureTest,
    shouldSupport_VoidFuture_ValueContinuations_ReturningValueFutures_WithContinuationErrors)
{
    std::promise<bool> called;

    struct test_error : public std::exception {
    };
    this->executor_->post([&called]() {
        make_ready_future()
            .then([]() -> Future<bool> { throw test_error{}; })
            .catch_error([&called](std::exception_ptr e) { called.set_exception(e); })
            .then([&called](bool value) { called.set_value(value); });
    });

    EXPECT_THROW(called.get_future().get(), test_error);
}

TYPED_TEST(
    FutureTest,
    shouldSupport_VoidFuture_ValueContinuations_ReturningValueFutures_WithExceptionalFutures)
{
    std::promise<bool> called;

    struct test_error : public std::exception {
    };
    this->executor_->post([&called]() {
        make_ready_future()
            .then([]() {
                std::exception_ptr e;
                try {
                    throw test_error{};
                } catch (...) {
                    e = std::current_exception();
                }
                return make_exceptional_future<bool>(e);
            })
            .catch_error([&called](std::exception_ptr e) { called.set_exception(e); })
            .then([&called](bool value) { called.set_value(value); });
    });

    EXPECT_THROW(called.get_future().get(), test_error);
}

// Future<T>::then(T -> void) -> Future<void>
TYPED_TEST(FutureTest, shouldSupport_ValueFuture_ValueContinuations_ReturningVoid)
{
    std::promise<void> called;

    this->executor_->post([&called]() {
        make_ready_future<int>(42).then([](int) {}).then([&called]() { called.set_value(); });
    });

    EXPECT_NO_THROW(called.get_future().get());
}

TYPED_TEST(
    FutureTest, shouldSupport_ValueFuture_ValueContinuations_ReturningVoid_WithContinuationErrors)
{
    std::promise<void> called;

    struct test_error : public std::exception {
    };
    this->executor_->post([&called]() {
        make_ready_future<int>(42)
            .then([](int) { throw test_error{}; })
            .catch_error([&called](std::exception_ptr e) { called.set_exception(e); })
            .then([&called]() { called.set_value(); });
    });

    EXPECT_THROW(called.get_future().get(), test_error);
}

TYPED_TEST(FutureTest, shouldSupport_ValueFuture_ConstLValueValueContinuations_ReturningVoid)
{
    std::promise<int> called;

    const auto lambda = [&called](int v) { called.set_value(v); };

    this->executor_->post([&lambda]() { make_ready_future(42).then(lambda); });

    EXPECT_EQ(42, called.get_future().get());
}

TYPED_TEST(FutureTest, shouldSupport_ValueFuture_NonConstLValueValueContinuations_ReturningVoid)
{
    std::promise<int> called;

    auto lambda = [&called](int v) mutable { called.set_value(v); };

    this->executor_->post([&lambda]() { make_ready_future(42).then(lambda); });

    EXPECT_EQ(42, called.get_future().get());
}

// Future<void>::then(void -> void) -> Future<void>
TYPED_TEST(FutureTest, shouldSupport_VoidFuture_VoidContinuations_ReturningVoid)
{
    std::promise<void> called;

    this->executor_->post([&called]() {
        make_ready_future().then([]() {}).then([&called]() { called.set_value(); });
    });

    EXPECT_NO_THROW(called.get_future().get());
}

TYPED_TEST(
    FutureTest, shouldSupport_VoidFuture_VoidContinuations_ReturningVoid_WithContinuationErrors)
{
    std::promise<void> called;

    struct test_error : public std::exception {
    };
    this->executor_->post([&called]() {
        make_ready_future()
            .then([]() -> Future<void> { throw test_error{}; })
            .catch_error([&called](std::exception_ptr e) { called.set_exception(e); })
            .then([&called]() { called.set_value(); });
    });

    EXPECT_THROW(called.get_future().get(), test_error);
}

TYPED_TEST(FutureTest, shouldSupport_VoidFuture_ConstLValueVoidContinuations_ReturningVoid)
{
    std::promise<void> called;

    const auto lambda = [&called]() { called.set_value(); };

    this->executor_->post([&lambda]() { make_ready_future().then(lambda); });

    EXPECT_NO_THROW(called.get_future().get());
}

TYPED_TEST(FutureTest, shouldSupport_VoidFuture_NonConstLValueVoidContinuations_ReturningVoid)
{
    std::promise<void> called;

    auto lambda = [&called]() mutable { called.set_value(); };

    this->executor_->post([&lambda]() { make_ready_future().then(lambda); });

    EXPECT_NO_THROW(called.get_future().get());
}

// Future<T>::then(T -> U) -> Future<U>
TYPED_TEST(FutureTest, shouldSupport_ValueFuture_ValueContinuations_ReturningValues)
{
    std::promise<bool> called;

    this->executor_->post([&called]() {
        make_ready_future<int>(42).then([](int) { return true; }).then([&called](bool value) {
            called.set_value(value);
        });
    });

    EXPECT_EQ(true, called.get_future().get());
}

TYPED_TEST(
    FutureTest, shouldSupport_ValueFuture_ValueContinuations_ReturningValues_WithContinuationErrors)
{
    std::promise<bool> called;

    struct test_error : public std::exception {
    };
    this->executor_->post([&called]() {
        make_ready_future<int>(42)
            .then([](int) -> Future<bool> { throw test_error{}; })
            .catch_error([&called](std::exception_ptr e) { called.set_exception(e); })
            .then([&called](bool value) { called.set_value(value); });
    });

    EXPECT_THROW(called.get_future().get(), test_error);
}

// Future<void>::then(void -> U) -> Future<U>
TYPED_TEST(FutureTest, shouldSupport_VoidFuture_ValueContinuations_ReturningValues)
{
    std::promise<bool> called;

    this->executor_->post([&called]() {
        make_ready_future().then([]() { return true; }).then([&called](bool value) {
            called.set_value(value);
        });
    });

    EXPECT_EQ(true, called.get_future().get());
}

TYPED_TEST(
    FutureTest, shouldSupport_VoidFuture_ValueContinuations_ReturningValues_WithContinuationErrors)
{
    std::promise<bool> called;

    struct test_error : public std::exception {
    };
    this->executor_->post([&called]() {
        make_ready_future()
            .then([]() -> Future<bool> { throw test_error{}; })
            .catch_error([&called](std::exception_ptr e) { called.set_exception(e); })
            .then([&called](bool value) { called.set_value(value); });
    });

    EXPECT_THROW(called.get_future().get(), test_error);
}

TYPED_TEST(FutureTest, shouldSupportResolvingPromisesWithLValues)
{
    std::promise<int> called;
    auto value = 5;

    this->executor_->post([&called, &value]() {
        auto lazy = make_lazy_future<int>();
        auto future = std::get<0>(lazy);
        auto promise = std::get<1>(lazy);

        future.then([&called](int v) { called.set_value(v); });

        promise.set_value(value);
    });

    EXPECT_EQ(called.get_future().get(), value);
}

TYPED_TEST(FutureTest, shouldSupportLValuesForMakeReadyFuture)
{
    std::promise<int> called;
    auto value = 5;

    this->executor_->post([&called, &value]() {
        auto future = make_ready_future<int>(value);

        future.then([&called](int v) { called.set_value(v); });
    });

    EXPECT_EQ(called.get_future().get(), value);
}

TYPED_TEST(FutureTest, thenShouldHoldExecutorReference)
{
    std::promise<void> thenCalled;

    auto lazy = make_lazy_future<void>();
    auto future = std::get<0>(lazy);
    auto promise = std::get<1>(lazy);

    this->executor_->post([&thenCalled, &future]() {
        future.then([]() {});
        thenCalled.set_value();
    });
    thenCalled.get_future().wait();
    // there is no guarantee here that the posted lambda has been executed completely,
    // so wait for a small amount of time,
    // we can't use termination_awaiter here cause in the successful case it would block
    // (future should still keep a reference, stored by future.then())
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    this->factory_.reset();
    this->executor_.reset();
    try {
        promise.set_value(); // this is going to crash if the executor isn't held by the future
                             // (then())
    } catch (const std::runtime_error&) { // this is going to throw because the executor is already
                                          // stopped, TODO: check if this is correct behavior
    }
}

/// FutureThrowingExecutorTest provides test cases that ensure futures behave correctly in case
/// underlying executors encounter runtime errors that prevent them to execute tasks that futures
/// schedule on them.
class FutureThrowingExecutorTest : public Test {
  public:
    FutureThrowingExecutorTest(std::tuple<asyncly::Future<void>, asyncly::Promise<void>> lazy)
        : promise_(std::get<1>(lazy))
        , future_(std::get<0>(lazy))
        , throwingExecutor_(asyncly::detail::ThrowingExecutor::create())
    {
    }
    FutureThrowingExecutorTest()
        : FutureThrowingExecutorTest(make_lazy_future<void>())
    {
    }

    void SetUp() override
    {
        asyncly::this_thread::set_current_executor(throwingExecutor_);
    }

    asyncly::Promise<void> promise_;
    asyncly::Future<void> future_;
    std::shared_ptr<asyncly::detail::ThrowingExecutor> throwingExecutor_;
};

TEST_F(FutureThrowingExecutorTest, throws_on_late_then)
{
    promise_.set_value();
    EXPECT_ANY_THROW(future_.then([]() { ADD_FAILURE(); }));
}

TEST_F(FutureThrowingExecutorTest, throws_on_late_set_value)
{
    future_.then([]() { ADD_FAILURE(); });
    EXPECT_ANY_THROW(promise_.set_value());
}

TEST_F(FutureThrowingExecutorTest, throws_on_late_catch_error)
{
    promise_.set_exception("intentional error");
    EXPECT_ANY_THROW(future_.catch_error([](auto) { ADD_FAILURE(); }));
}

TEST_F(FutureThrowingExecutorTest, throws_on_late_set_exception)
{
    future_.catch_error([](auto) { ADD_FAILURE(); });
    EXPECT_ANY_THROW(promise_.set_exception("intentional error"));
}

}
