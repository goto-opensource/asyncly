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

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <future>

#include "asyncly/executor/IExecutor.h"

namespace asyncly::test {

using namespace ::testing;

struct CallThreadIdReporter {
    CallThreadIdReporter(
        std::shared_ptr<std::promise<std::thread::id>> threadId,
        std::shared_ptr<std::promise<std::thread::id>> destructorThreadId)
        : threadId_(threadId)
        , destructorThreadId_(destructorThreadId)
    {
    }

    CallThreadIdReporter(const CallThreadIdReporter&) = delete;
    CallThreadIdReporter& operator=(const CallThreadIdReporter&) = delete;

    CallThreadIdReporter(CallThreadIdReporter&& other)
    {
        // todo replace by = default for vs 2015
        *this = std::move(other);
    }
    CallThreadIdReporter& operator=(CallThreadIdReporter&& other)
    {
        threadId_ = std::move(other.threadId_);
        destructorThreadId_ = std::move(other.destructorThreadId_);
        return *this;
    }

    ~CallThreadIdReporter()
    {
        if (destructorThreadId_) {
            destructorThreadId_->set_value(std::this_thread::get_id());
        }
    }

    void operator()()
    {
        if (threadId_) {
            threadId_->set_value(std::this_thread::get_id());
        }
    }

    std::shared_ptr<std::promise<std::thread::id>> threadId_;
    std::shared_ptr<std::promise<std::thread::id>> destructorThreadId_;
};

template <typename TExecutorFactory, class ExecutorInterface>
class ExecutorCommonTestBase : public Test {
  public:
    ExecutorCommonTestBase()
        : executor_(factory_.create())
    {
    }
    TExecutorFactory factory_;
    std::shared_ptr<IExecutor> executor_;
    std::chrono::milliseconds delay_{ 20 };
    std::chrono::seconds timeout_{ 5 };
};

template <typename TExecutorFactory>
class ExecutorCommonTest : public ExecutorCommonTestBase<TExecutorFactory, IExecutor> { };

TYPED_TEST_SUITE_P(ExecutorCommonTest);

TYPED_TEST_P(ExecutorCommonTest, shouldDispatchASingleMessage)
{
    std::promise<void> promise;
    this->executor_->post([&promise]() { promise.set_value(); });
    ASSERT_EQ(std::future_status::ready, promise.get_future().wait_for(this->timeout_));
}

TYPED_TEST_P(ExecutorCommonTest, shouldDispatchMultipleMessages)
{
    std::vector<std::promise<void>> promises;
    promises.resize(200);
    std::for_each(promises.begin(), promises.end(), [this](std::promise<void>& promise) {
        this->executor_->post([&promise]() { promise.set_value(); });
    });
    std::for_each(promises.begin(), promises.end(), [this](std::promise<void>& promise) {
        EXPECT_EQ(std::future_status::ready, promise.get_future().wait_for(this->timeout_));
    });
}

TYPED_TEST_P(ExecutorCommonTest, shouldRejectBadClosure)
{
    using FunctionType = void();
    FunctionType* invalidFunction = nullptr;
    EXPECT_ANY_THROW(this->executor_->post(invalidFunction));
}

TYPED_TEST_P(ExecutorCommonTest, shouldExecuteClosuresInWorkerThread)
{
    using thread_id = std::thread::id;
    thread_id testThreadId = std::this_thread::get_id();
    std::promise<thread_id> workerThreadId;
    this->executor_->post(
        [&workerThreadId]() { workerThreadId.set_value(std::this_thread::get_id()); });
    auto future = workerThreadId.get_future();
    EXPECT_EQ(std::future_status::ready, future.wait_for(this->timeout_));
    EXPECT_NE(future.get(), testThreadId);
}

TYPED_TEST_P(ExecutorCommonTest, shouldDestructClosureInDispatchingThread)
{
    for (int i = 0; i < 1000; i++) {
        auto callPromise = std::make_shared<std::promise<std::thread::id>>();
        auto destructorPromise = std::make_shared<std::promise<std::thread::id>>();

        this->executor_->post(CallThreadIdReporter{ callPromise, destructorPromise });

        EXPECT_EQ(callPromise->get_future().get(), destructorPromise->get_future().get());
    }
}

TYPED_TEST_P(ExecutorCommonTest, shouldFetchCurrentExecutorWhenInPostTask)
{
    std::promise<IExecutorPtr> taskExecutor;

    this->executor_->post([&taskExecutor]() {
        taskExecutor.set_value(::asyncly::this_thread::get_current_executor());
    });

    EXPECT_EQ(this->executor_, taskExecutor.get_future().get());
}

TYPED_TEST_P(ExecutorCommonTest, shouldFetchCurrentExecutorWhenInPostAtTask)
{
    std::promise<IExecutorPtr> taskExecutor;

    auto timeAtDispatch = clock_type::now();
    this->executor_->post_at(timeAtDispatch + this->delay_, [&taskExecutor]() {
        taskExecutor.set_value(::asyncly::this_thread::get_current_executor());
    });

    EXPECT_EQ(this->executor_, taskExecutor.get_future().get());
}

TYPED_TEST_P(ExecutorCommonTest, shouldFetchCurrentExecutorWhenInPostAfterTask)
{
    std::promise<IExecutorPtr> taskExecutor;

    this->executor_->post_after(this->delay_, [&taskExecutor]() {
        taskExecutor.set_value(::asyncly::this_thread::get_current_executor());
    });

    EXPECT_EQ(this->executor_, taskExecutor.get_future().get());
}

TYPED_TEST_P(ExecutorCommonTest, shouldFetchCurrentExecutorWhenInTaskDestruction)
{
    std::promise<IExecutorPtr> taskExecutor;

    auto ptr = std::unique_ptr<int, std::function<void(int*)>>(new int(1), [&taskExecutor](int* p) {
        taskExecutor.set_value(::asyncly::this_thread::get_current_executor());
        delete p;
    });
    this->executor_->post([ptr{ std::move(ptr) }]() {});

    EXPECT_EQ(this->executor_, taskExecutor.get_future().get());
}

TYPED_TEST_P(ExecutorCommonTest, shouldThrowWhenFetchingCurrentExecutorWhenNotInExecutorContext)
{
    EXPECT_THROW(::asyncly::this_thread::get_current_executor(), std::exception);
}

REGISTER_TYPED_TEST_SUITE_P(
    ExecutorCommonTest,
    shouldDispatchASingleMessage,
    shouldDispatchMultipleMessages,
    shouldRejectBadClosure,
    shouldExecuteClosuresInWorkerThread,
    shouldDestructClosureInDispatchingThread,
    shouldFetchCurrentExecutorWhenInPostTask,
    shouldFetchCurrentExecutorWhenInPostAtTask,
    shouldFetchCurrentExecutorWhenInPostAfterTask,
    shouldFetchCurrentExecutorWhenInTaskDestruction,
    shouldThrowWhenFetchingCurrentExecutorWhenNotInExecutorContext);

//////////////////////////////////////////
// scheduledExecutor tests
//////////////////////////////////////////

template <typename TExecutorFactory>
class ScheduledExecutorCommonTest : public ExecutorCommonTestBase<TExecutorFactory, IExecutor> { };

TYPED_TEST_SUITE_P(ScheduledExecutorCommonTest);

TYPED_TEST_P(ScheduledExecutorCommonTest, shouldScheduleClosureWithRelativeTimeOffset)
{
    auto timeAtDispatch = clock_type::now();
    std::promise<clock_type::time_point> timeAtExecutionPromise;

    this->executor_->post_after(this->delay_, [&timeAtExecutionPromise]() {
        timeAtExecutionPromise.set_value(clock_type::now());
    });

    auto timeAtExecution = timeAtExecutionPromise.get_future().get();
    EXPECT_GE(timeAtExecution - timeAtDispatch, this->delay_);
}

TYPED_TEST_P(ScheduledExecutorCommonTest, shouldScheduleClosureWithAbsoluteTime)
{
    auto timeAtDispatch = clock_type::now();
    std::promise<clock_type::time_point> timeAtExecutionPromise;

    this->executor_->post_at(timeAtDispatch + this->delay_, [&timeAtExecutionPromise]() {
        timeAtExecutionPromise.set_value(clock_type::now());
    });

    auto timeAtExecution = timeAtExecutionPromise.get_future().get();
    EXPECT_GE(timeAtExecution - timeAtDispatch, this->delay_);
}

TYPED_TEST_P(ScheduledExecutorCommonTest, shouldScheduleClosureWithPeriodicTime)
{
    auto timeAtDispatch = this->executor_->now();
    std::promise<clock_type::time_point> timeAtExecutionPromise;

    struct State {
        bool compareAndSwap()
        {
            std::lock_guard<std::mutex> lock{ mutex };
            if (hasBeenRunOnce == false) {
                hasBeenRunOnce = true;
                return true;
            }

            return false;
        }

        std::mutex mutex;
        bool hasBeenRunOnce = false;
    };

    auto state = std::make_shared<State>();

    auto autoCancelable
        = this->executor_->post_periodically(this->delay_, [&timeAtExecutionPromise, state]() {
              if (state->compareAndSwap()) {
                  timeAtExecutionPromise.set_value(clock_type::now());
              }
          });

    auto timeAtExecution = timeAtExecutionPromise.get_future().get();
    EXPECT_GE(timeAtExecution - timeAtDispatch, this->delay_);
    autoCancelable.reset();
}

TYPED_TEST_P(ScheduledExecutorCommonTest, shouldExecuteBeforeNewestExpires)
{
    const auto timeAtDispatch = clock_type::now();

    const int delayMs = 5;

    int i = 0;

    std::promise<int> firstPromise;
    std::promise<int> secondPromise;

    this->executor_->post_at(
        timeAtDispatch + (1 * std::chrono::milliseconds(delayMs)),
        [&i, &firstPromise]() { firstPromise.set_value(i++); });
    this->executor_->post_at(
        timeAtDispatch + (2 * std::chrono::milliseconds(delayMs)),
        [&i, &secondPromise]() { secondPromise.set_value(i++); });

    // scheduled in the very distant future
    auto timerTask = this->executor_->post_at(timeAtDispatch + std::chrono::hours(1), []() {});

    auto firstFuture = firstPromise.get_future();
    auto secondFuture = secondPromise.get_future();

    ASSERT_EQ(std::future_status::ready, firstFuture.wait_for(std::chrono::seconds(5)));
    ASSERT_EQ(std::future_status::ready, secondFuture.wait_for(std::chrono::seconds(5)));

    EXPECT_EQ(0, firstFuture.get());
    EXPECT_EQ(1, secondFuture.get());

    // this is required to be able to shutdown:
    // otherwise there is a timer task owning the executor in the io_context
    // the executor dtor depends on the (running) io_context, the io_context holds the executor
    // (hidden in the timer task), there seems to be no way to cancel all pending timers of a
    // io_context w/o destroying it
    // TODO: fix the executor shutdown!
    timerTask->cancel();
}

TYPED_TEST_P(ScheduledExecutorCommonTest, shouldCancelClosureWithRelativeTimeOffset)
{
    std::promise<void> continueQueue;
    std::shared_future<void> continueQueueFuture(continueQueue.get_future());
    std::promise<void> actionPerformed;

    this->executor_->post([continueQueueFuture]() { continueQueueFuture.get(); });
    auto cancelable = this->executor_->post_after(
        this->delay_, [&actionPerformed]() { actionPerformed.set_value(); });
    cancelable->cancel();
    continueQueue.set_value();
    EXPECT_EQ(
        std::future_status::timeout,
        actionPerformed.get_future().wait_for(
            std::chrono::milliseconds{ 2 * this->delay_.count() }));
}

TYPED_TEST_P(ScheduledExecutorCommonTest, shouldCancelClosureWithPeriodic)
{
    std::promise<void> continueQueue;
    std::shared_future<void> continueQueueFuture(continueQueue.get_future());
    std::promise<void> actionPerformed;

    this->executor_->post([continueQueueFuture]() { continueQueueFuture.get(); });
    auto autoCancelable = this->executor_->post_periodically(
        this->delay_, [&actionPerformed]() { actionPerformed.set_value(); });
    autoCancelable.reset();
    continueQueue.set_value();
    EXPECT_EQ(
        std::future_status::timeout,
        actionPerformed.get_future().wait_for(
            std::chrono::milliseconds{ 2 * this->delay_.count() }));
}

TYPED_TEST_P(ScheduledExecutorCommonTest, shouldReschedulePeriodic)
{
    std::promise<int> actionPerformed;

    struct State {
        int incrementAndGet()
        {
            std::lock_guard<std::mutex> lock{ this->mutex };
            this->counter++;
            return this->counter;
        }
        int counter = 0;
        std::mutex mutex;
    };

    auto state = std::make_shared<State>();

    auto autoCancelable
        = this->executor_->post_periodically(this->delay_, [&actionPerformed, state]() {
              if (state->incrementAndGet() == 2) {
                  actionPerformed.set_value(2);
              }
          });

    auto result = actionPerformed.get_future().get();
    ASSERT_GE(result, 2);
    autoCancelable.reset();
}

TYPED_TEST_P(ScheduledExecutorCommonTest, shouldNotCrashThroughCancelAfterFinishedTask)
{
    std::promise<void> actionPerformed;

    auto cancelable = this->executor_->post_after(
        this->delay_, [&actionPerformed]() { actionPerformed.set_value(); });

    actionPerformed.get_future().wait();
    cancelable->cancel();
}

TYPED_TEST_P(
    ScheduledExecutorCommonTest,
    shouldCancelAndDeleteClosureAndReleaseExecutorReferenceSoThatThreadsFinish)
{
    std::promise<void> actionPerformed;
    auto executor = this->executor_;

    auto cancelable = this->executor_->post_after(
        std::chrono::hours(1),
        [&actionPerformed, executor{ std::move(executor) }]() { actionPerformed.set_value(); });

    cancelable->cancel();
    EXPECT_EQ(
        std::future_status::timeout,
        actionPerformed.get_future().wait_for(
            std::chrono::milliseconds{ 2 * this->delay_.count() }));
}

TYPED_TEST_P(
    ScheduledExecutorCommonTest,
    shouldCancelAndDeleteClosureAndReleaseExecutorReferenceSoThatThreadsFinishWithPeriodic)
{
    std::promise<void> actionPerformed;
    auto executor = this->executor_;

    auto autoCancelable = this->executor_->post_periodically(
        std::chrono::hours(1),
        [&actionPerformed, executor{ std::move(executor) }]() { actionPerformed.set_value(); });

    autoCancelable.reset();
    EXPECT_EQ(
        std::future_status::timeout,
        actionPerformed.get_future().wait_for(
            std::chrono::milliseconds{ 2 * this->delay_.count() }));
}

REGISTER_TYPED_TEST_SUITE_P(
    ScheduledExecutorCommonTest,
    shouldScheduleClosureWithRelativeTimeOffset,
    shouldScheduleClosureWithAbsoluteTime,
    shouldScheduleClosureWithPeriodicTime,
    shouldExecuteBeforeNewestExpires,
    shouldCancelClosureWithRelativeTimeOffset,
    shouldCancelClosureWithPeriodic,
    shouldReschedulePeriodic,
    shouldNotCrashThroughCancelAfterFinishedTask,
    shouldCancelAndDeleteClosureAndReleaseExecutorReferenceSoThatThreadsFinish,
    shouldCancelAndDeleteClosureAndReleaseExecutorReferenceSoThatThreadsFinishWithPeriodic);
} // namespace asyncly::test
