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

#include <numeric>

#include "gmock/gmock.h"

#include "asyncly/executor/CurrentExecutor.h"
#include "asyncly/observable/Observable.h"

#include "StrandImplTestFactory.h"
#include "asyncly/test/ExecutorTestFactories.h"

namespace asyncly {

using namespace testing;

template <typename TExecutorFactory> class ObservableTest : public Test {
  public:
    ObservableTest()
        : executor_(factory_.create())
    {
    }

    void SetUp() override
    {
    }

    TExecutorFactory factory_;
    TExecutorFactory factory2_;
    std::shared_ptr<IExecutor> executor_;
};

using ExecutorFactoryTypes = ::testing::Types<
    asyncly::test::AsioExecutorFactory<>,
    asyncly::test::DefaultExecutorFactory<>,
    asyncly::test::StrandImplTestFactory<>>;

TYPED_TEST_SUITE(ObservableTest, ExecutorFactoryTypes);

TYPED_TEST(ObservableTest, shouldPropagateASimpleValue)
{
    std::promise<int> value;

    const auto onValue = [&value](int v) mutable { value.set_value(v); };

    this->executor_->post([onValue]() {
        auto observable = make_lazy_observable<int>([](auto subscriber) {
            subscriber.pushValue(42);
            subscriber.complete();
        });

        observable.subscribe(onValue);
    });

    EXPECT_EQ(42, value.get_future().get());
}

TYPED_TEST(ObservableTest, shouldPropagateMultipleValues)
{
    std::promise<void> done;
    const std::vector<int> pushedValues = { 2, 3, 5, 7, 11, 13 };
    std::vector<int> receivedValues;

    const auto onValue = [&done, &receivedValues, &pushedValues](int v) mutable {
        receivedValues.push_back(v);
        if (receivedValues.size() == pushedValues.size()) {
            done.set_value();
        }
    };

    this->executor_->post([executor = this->executor_, onValue, &pushedValues]() mutable {
        auto observable = make_lazy_observable<int>(
            [&pushedValues, executor](auto subscriber) mutable {
                for (auto value : pushedValues) {
                    executor->post([value, subscriber]() mutable { subscriber.pushValue(value); });
                }
            });
        observable.subscribe(onValue);
    });

    done.get_future().wait();

    EXPECT_THAT(receivedValues, ContainerEq(pushedValues));
}

TYPED_TEST(ObservableTest, shouldPropagateCompletion)
{
    std::promise<void> completed;

    const auto onValue = [](int) mutable {};
    const auto onError = [](std::exception_ptr) {};
    const auto onCompletion = [&completed]() { completed.set_value(); };

    this->executor_->post([onValue, onError, onCompletion]() mutable {
        auto observable = make_lazy_observable<int>([](auto subscriber) { subscriber.complete(); });
        observable.subscribe(onValue, onError, onCompletion);
    });

    completed.get_future().wait();
}

TYPED_TEST(ObservableTest, shouldPropagateErrors)
{
    std::promise<std::exception_ptr> errored;
    struct MyError : public std::exception {
    };

    const auto onError = [&errored](std::exception_ptr e) { errored.set_value(e); };

    this->executor_->post([onError]() mutable {
        auto observable = make_lazy_observable<int>(
            [](auto subscriber) { subscriber.pushError(std::make_exception_ptr(MyError{})); });

        observable.subscribe(nullptr, onError);
    });

    try {
        std::rethrow_exception(errored.get_future().get());
    } catch (MyError&) {
        SUCCEED();
    } catch (...) {
        FAIL();
    }
}

TYPED_TEST(ObservableTest, shouldNotReceiveValuesIfUnsubscribed)
{
    std::promise<void> done;

    struct MockValueReceiver {
        MOCK_METHOD0(valueReceived, void());
    };

    MockValueReceiver valueReceiver{};

    const auto onValue = [&valueReceiver](int) mutable { valueReceiver.valueReceived(); };

    EXPECT_CALL(valueReceiver, valueReceived()).Times(0);

    this->executor_->post([onValue, &done]() mutable {
        auto observable
            = make_lazy_observable<int>([](auto subscriber) { subscriber.pushValue(42); });

        auto subscription = observable.subscribe(onValue);
        subscription.cancel();
        done.set_value();
    });

    done.get_future().wait();
}

TYPED_TEST(ObservableTest, shouldNotReceiveCompletionsIfUnsubscribed)
{
    std::promise<void> done;

    struct MockValueReceiver {
        MOCK_METHOD0(completionReceived, void());
    };

    MockValueReceiver valueReceiver{};

    const auto onCompletion = [&valueReceiver]() mutable { valueReceiver.completionReceived(); };

    EXPECT_CALL(valueReceiver, completionReceived()).Times(0);

    this->executor_->post([onCompletion, &done]() mutable {
        auto observable = make_lazy_observable<int>([](auto subscriber) { subscriber.complete(); });

        auto subscription = observable.subscribe(nullptr, nullptr, onCompletion);
        subscription.cancel();
        done.set_value();
    });
    done.get_future().wait();
}

TYPED_TEST(ObservableTest, shouldNotReceiveErrorsIfUnsubscribed)
{
    std::promise<void> done;
    struct MockValueReceiver {
        MOCK_METHOD0(errorReceived, void());
    };

    MockValueReceiver valueReceiver{};

    const auto onError
        = [&valueReceiver](std::exception_ptr) mutable { valueReceiver.errorReceived(); };

    EXPECT_CALL(valueReceiver, errorReceived()).Times(0);

    auto doneFuture = done.get_future();

    this->executor_->post([donePromise = std::move(done), onError]() mutable {
        auto observable = make_lazy_observable<int>([](auto subscriber) {
            subscriber.pushError(std::make_exception_ptr(std::runtime_error{ "some error" }));
        });

        auto subscription = observable.subscribe(nullptr, onError);
        subscription.cancel();

        donePromise.set_value();
    });

    doneFuture.wait();
}

TYPED_TEST(ObservableTest, shouldPropagateToMultipleSubscribers)
{
    std::promise<void> done1;
    std::promise<void> done2;
    std::vector<int> pushedValues = { 2, 3, 5, 7, 11, 13 };
    std::vector<int> receivedValues1;
    std::vector<int> receivedValues2;

    const auto onValue1 = [&done1, &receivedValues1, &pushedValues](int v) mutable {
        receivedValues1.push_back(v);
        if (receivedValues1.size() == pushedValues.size()) {
            done1.set_value();
        }
    };

    const auto onValue2 = [&done2, &receivedValues2, &pushedValues](int v) mutable {
        receivedValues2.push_back(v);
        if (receivedValues2.size() == pushedValues.size()) {
            done2.set_value();
        }
    };

    this->executor_->post([onValue1, onValue2, executor = this->executor_, pushedValues]() mutable {
        auto observable = make_lazy_observable<int>(
            [pushedValues, executor](auto subscriber) mutable {
                for (auto value : pushedValues) {
                    executor->post([value, subscriber]() mutable { subscriber.pushValue(value); });
                }
                subscriber.complete();
            });

        observable.subscribe(onValue1);
        observable.subscribe(onValue2);
    });

    done1.get_future().wait();
    done2.get_future().wait();

    EXPECT_THAT(receivedValues1, ContainerEq(pushedValues));
    EXPECT_THAT(receivedValues2, ContainerEq(pushedValues));
}

TYPED_TEST(ObservableTest, shouldCallIntoSubscribersExecutor)
{
    auto producerExecutor = this->factory_.create();
    auto subscriberExecutor = this->factory2_.create();

    std::promise<asyncly::IExecutorPtr> subscriberExecutorPromise;
    std::promise<asyncly::IExecutorPtr> producerExecutorPromise;

    const auto onValue = [&subscriberExecutorPromise](int) mutable {
        subscriberExecutorPromise.set_value(asyncly::this_thread::get_current_executor());
    };

    std::promise<void> subscribed;
    producerExecutor->post(
        [&producerExecutorPromise, &subscribed, onValue, subscriberExecutor]() mutable {
            auto observable = make_lazy_observable<int>(
                [&producerExecutorPromise](auto subscriber) mutable {
                    producerExecutorPromise.set_value(asyncly::this_thread::get_current_executor());
                    subscriber.pushValue(5);
                });
            subscriberExecutor->post([&subscribed, onValue, observable]() mutable {
                observable.subscribe(onValue);
                subscribed.set_value();
            });
        });

    subscribed.get_future().wait();

    auto observedSubscriberExecutor = subscriberExecutorPromise.get_future().get();
    auto observedproducerExecutor = producerExecutorPromise.get_future().get();

    EXPECT_EQ(subscriberExecutor, observedSubscriberExecutor);
    EXPECT_EQ(producerExecutor, observedproducerExecutor);
}

TYPED_TEST(ObservableTest, shouldThrowWhenPushingAfterCompletion)
{
    std::promise<void> done;

    this->executor_->post([&done]() mutable {
        auto observable = make_lazy_observable<int>([&done](auto subscriber) mutable {
            subscriber.complete();
            try {
                subscriber.pushValue(42);
            } catch (...) {
                done.set_exception(std::current_exception());
            }
        });
        observable.subscribe(nullptr);
    });

    EXPECT_THROW(done.get_future().get(), std::exception);
}

TYPED_TEST(ObservableTest, shouldSupportMap)
{
    std::promise<void> done;
    const std::vector<int> pushedValues = { 2, 3, 5, 7, 11, 13 };
    std::vector<bool> receivedValues;

    const auto onValue = [&done, &receivedValues, &pushedValues](bool v) mutable {
        receivedValues.push_back(v);
        if (receivedValues.size() == pushedValues.size()) {
            done.set_value();
        }
    };

    auto isEven = [](int v) { return v % 2 == 0; };

    std::vector<bool> expectedValues;
    std::transform(
        pushedValues.begin(), pushedValues.end(), std::back_inserter(expectedValues), isEven);
    this->executor_->post([executor = this->executor_, onValue, &pushedValues, &isEven]() mutable {
        auto observable
            = make_lazy_observable<int>([&pushedValues, executor](auto subscriber) mutable {
                  for (auto value : pushedValues) {
                      subscriber.pushValue(value);
                  }
              }).map(isEven);
        observable.subscribe(onValue);
    });

    done.get_future().wait();

    EXPECT_THAT(receivedValues, ContainerEq(expectedValues));
}

TYPED_TEST(ObservableTest, shouldSupportMapVoid)
{
    std::promise<void> done;
    auto numberOfValues = 5u;
    std::vector<bool> receivedValues;
    std::atomic<int> counter{ 0 };

    auto isEven = [](int v) { return v % 2 == 0; };
    auto isEvenMapped = [&counter]() { return counter++ % 2 == 0; };

    const auto onValue = [&done, &receivedValues, numberOfValues](bool value) mutable {
        receivedValues.push_back(value);
        if (receivedValues.size() == numberOfValues) {
            done.set_value();
        }
    };

    std::vector<bool> expectedValues;
    std::vector<int> counterValues(numberOfValues);
    std::iota(counterValues.begin(), counterValues.end(), 0);
    std::transform(
        counterValues.begin(), counterValues.end(), std::back_inserter(expectedValues), isEven);
    this->executor_->post(
        [executor = this->executor_, onValue, &isEvenMapped, numberOfValues]() mutable {
            auto observable
                = make_lazy_observable<void>([executor, numberOfValues](auto subscriber) mutable {
                      for (unsigned i = 0; i < numberOfValues; i++) {
                          subscriber.pushValue();
                      }
                  }).map(isEvenMapped);
            observable.subscribe(onValue);
        });

    done.get_future().wait();

    EXPECT_THAT(receivedValues, ContainerEq(expectedValues));
}

TYPED_TEST(ObservableTest, shouldSupportFilter)
{
    std::promise<void> done;
    const std::vector<int> pushedValues = { 2, 3, 5, 7, 8, 10, 11, 12, 13 };
    std::vector<int> receivedValues;

    auto isEven = [](int v) { return v % 2 == 0; };
    std::vector<int> expectedValues;
    std::copy_if(
        pushedValues.begin(), pushedValues.end(), std::back_inserter(expectedValues), isEven);

    const auto onValue = [&done, &receivedValues, &expectedValues](int v) mutable {
        receivedValues.push_back(v);
        if (receivedValues.size() == expectedValues.size()) {
            done.set_value();
        }
    };

    this->executor_->post([executor = this->executor_, onValue, &pushedValues, &isEven]() mutable {
        auto observable
            = make_lazy_observable<int>([&pushedValues, executor](auto subscriber) mutable {
                  for (auto value : pushedValues) {
                      subscriber.pushValue(value);
                  }
              }).filter(isEven);
        observable.subscribe(onValue);
    });

    done.get_future().wait();

    EXPECT_THAT(receivedValues, ContainerEq(expectedValues));
}

TYPED_TEST(ObservableTest, shouldPropagateVoid)
{
    std::promise<void> value;

    const auto onValue = [&value]() mutable { value.set_value(); };

    this->executor_->post([onValue]() {
        auto observable = make_lazy_observable<void>([](auto subscriber) {
            subscriber.pushValue();
            subscriber.complete();
        });

        observable.subscribe(onValue);
    });

    EXPECT_NO_THROW(value.get_future().get());
}

TYPED_TEST(ObservableTest, shouldPropagateVoidCompletion)
{
    std::promise<void> complete;

    const auto onValue = []() {};
    const auto onError = [](auto) {};
    const auto onComplete = [&complete]() mutable { complete.set_value(); };

    this->executor_->post([onValue, onError, onComplete]() {
        auto observable
            = make_lazy_observable<void>([](auto subscriber) { subscriber.complete(); });

        observable.subscribe(onValue, onError, onComplete);
    });

    EXPECT_NO_THROW(complete.get_future().get());
}

TYPED_TEST(ObservableTest, shouldPropagateVoidError)
{
    std::promise<void> error;

    struct MyError : public std::exception {
    };

    const auto onValue = []() {};
    const auto onError = [&error](auto e) mutable { error.set_exception(e); };

    this->executor_->post([onValue, onError]() {
        auto observable = make_lazy_observable<void>(
            [](auto subscriber) { subscriber.pushError(std::make_exception_ptr(MyError{})); });

        observable.subscribe(onValue, onError);
    });

    EXPECT_THROW(error.get_future().get(), MyError);
}

TYPED_TEST(ObservableTest, shouldSupportScanVoidObservables)
{
    int numberOfInvokations = 5;
    std::promise<int> complete;

    const auto onValue = [numberOfInvokations, &complete](int counter) {
        if (counter == numberOfInvokations) {
            complete.set_value(numberOfInvokations);
        }
    };

    this->executor_->post([onValue, numberOfInvokations]() {
        auto observable = make_lazy_observable<void>([numberOfInvokations](auto subscriber) {
            for (int i = 0; i < numberOfInvokations; i++) {
                subscriber.pushValue();
            }
            subscriber.complete();
        });

        observable.scan([](int counter) { return counter + 1; }, 0).subscribe(onValue);
    });

    EXPECT_EQ(complete.get_future().get(), numberOfInvokations);
}

TYPED_TEST(ObservableTest, shouldSupportScanValueObservables)
{
    int numberOfInvokations = 5;
    std::promise<int> complete;

    struct State {
        int counter;
        int sum;
    };

    const auto onValue = [numberOfInvokations, &complete](State state) {
        if (state.counter == numberOfInvokations) {
            complete.set_value(state.sum);
        }
    };

    this->executor_->post([onValue, numberOfInvokations]() {
        auto observable = make_lazy_observable<int>([numberOfInvokations](auto subscriber) {
            for (int i = 0; i < numberOfInvokations; i++) {
                subscriber.pushValue(i);
            }
            subscriber.complete();
        });

        observable
            .scan(
                [](State state, int value) {
                    return State{ state.counter + 1, state.sum + value };
                },
                State{ 0, 0 })
            .subscribe(onValue);
    });

    auto expectedSum = 0;
    for (int i = 0; i < numberOfInvokations; i++) {
        expectedSum += i;
    }

    EXPECT_EQ(complete.get_future().get(), expectedSum);
}
}
