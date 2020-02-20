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

#include <future>
#include <thread>

#include "gmock/gmock.h"

#include "asyncly/Wrap.h"
#include "asyncly/executor/ThreadPoolExecutorController.h"

#include "asyncly/test/ExecutorTestFactories.h"

namespace asyncly {

using namespace testing;

template <typename TExecutorFactory> class WrapTest : public Test {
  public:
    WrapTest()
        : executor_(factory_.create())
    {
    }

    TExecutorFactory factory_;
    std::shared_ptr<IExecutor> executor_;
};

using ExecutorFactoryTypes = ::testing::
    Types<asyncly::test::AsioExecutorFactory<>, asyncly::test::DefaultExecutorFactory<>>;

TYPED_TEST_SUITE(WrapTest, ExecutorFactoryTypes);

struct Callback : public std::enable_shared_from_this<Callback> {
    MOCK_METHOD0(callback, void());
    MOCK_METHOD1(callbackWithReturn, int(int));
};

TYPED_TEST(WrapTest, shouldWrapPost)
{
    std::promise<std::thread::id> called;
    auto wrapped = asyncly::wrap_post(
        this->executor_, [&called]() { called.set_value(std::this_thread::get_id()); });

    auto future = called.get_future();
    // make sure nothing is executed before being called
    EXPECT_NE(future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);

    wrapped();
    // make sure the function is posted to the executor (executed in another thread in this
    // instance)
    EXPECT_NE(future.get(), std::this_thread::get_id());
}

TYPED_TEST(WrapTest, shouldWrapPostWithArguments)
{
    std::promise<int> called;

    auto x = 1;
    auto y = 2;

    auto wrapped
        = asyncly::wrap_post(this->executor_, [&called](int a, int b) { called.set_value(a + b); });

    auto future = called.get_future();
    wrapped(x, y);
    EXPECT_EQ(future.get(), x + y);
}

TYPED_TEST(WrapTest, shouldWeakWrapPost)
{
    std::promise<void> done;
    auto callback = std::make_shared<Callback>();

    EXPECT_CALL(*callback, callback()).Times(0);

    auto wrapped = asyncly::wrap_weak_post_ignore(
        this->executor_, callback, [&done](const std::shared_ptr<Callback>& cb) {
            cb->callback();
            done.set_value();
        });

    Mock::VerifyAndClearExpectations(callback.get());

    EXPECT_CALL(*callback, callback());
    wrapped();
    done.get_future().wait();
}

TYPED_TEST(WrapTest, shouldWeakWrapPostWithArguments)
{
    std::promise<int> done;
    auto callback = std::make_shared<NiceMock<Callback>>();
    int x = 1;
    int y = 2;

    auto wrapped = asyncly::wrap_weak_post_ignore(
        this->executor_, callback, [&done](const std::shared_ptr<Callback>& cb, int a, int b) {
            cb->callback();
            done.set_value(a + b);
        });

    wrapped(x, y);
    EXPECT_EQ(done.get_future().get(), x + y);
}

TYPED_TEST(WrapTest, shouldNotCallExpiredWeakWrapPost)
{
    std::promise<void> done;
    auto callback = std::make_shared<Callback>();

    EXPECT_CALL(*callback, callback()).Times(0);

    auto wrapped = asyncly::wrap_weak_post_ignore(
        this->executor_, callback, [](const std::shared_ptr<Callback>& cb) { cb->callback(); });

    callback.reset();
    wrapped();
    this->executor_->post([&done]() { done.set_value(); });
    done.get_future().wait();
}

TYPED_TEST(WrapTest, shouldCustomErrorWithExpiredWeakWrapPost)
{
    std::promise<void> errored;
    auto callback = std::make_shared<Callback>();

    EXPECT_CALL(*callback, callback()).Times(0);

    auto wrapped = asyncly::wrap_weak_post_with_custom_error(
        this->executor_,
        callback,
        [](const std::shared_ptr<Callback>& cb) { cb->callback(); },
        [&errored]() { errored.set_value(); });

    EXPECT_CALL(*callback, callback()).Times(0);
    callback.reset();
    wrapped();
    errored.get_future().wait();
}

TYPED_TEST(WrapTest, shouldWeakWrap)
{
    auto callback = std::make_shared<Callback>();

    auto wrapped
        = asyncly::wrap_weak(callback, [](auto aliveCallback) { aliveCallback->callback(); });

    EXPECT_CALL(*callback, callback());
    wrapped();
}

TYPED_TEST(WrapTest, shouldWeakWrapWithReturnValues)
{
    auto callback = std::make_shared<NiceMock<Callback>>();
    EXPECT_CALL(*callback, callbackWithReturn(_)).WillOnce(ReturnArg<0>());

    auto wrapped = asyncly::wrap_weak(
        callback, [](auto aliveCallback, int a) { return aliveCallback->callbackWithReturn(a); });

    EXPECT_EQ(12, wrapped(12));
}

TYPED_TEST(WrapTest, shouldErrorOnDeadWeakWrap)
{
    auto callback = std::make_shared<Callback>();

    auto wrapped
        = asyncly::wrap_weak(callback, [](auto aliveCallback) { aliveCallback->callback(); });

    EXPECT_CALL(*callback, callback()).Times(0);
    callback.reset();
    EXPECT_THROW(wrapped(), std::runtime_error);
}

TYPED_TEST(WrapTest, shouldCustomErrorOnDeadWeakWrap)
{
    auto callback = std::make_shared<Callback>();

    struct MyError : public std::exception {
    };

    auto wrapped = asyncly::wrap_weak_with_custom_error(
        callback, [](auto aliveCallback) { aliveCallback->callback(); }, []() { throw MyError{}; });

    EXPECT_CALL(*callback, callback()).Times(0);
    callback.reset();
    EXPECT_THROW(wrapped(), MyError);
}

TYPED_TEST(WrapTest, shouldIgnoreOnDeadWeakWrapIgnore)
{
    auto callback = std::make_shared<Callback>();

    auto wrapped = asyncly::wrap_weak_ignore(
        callback, [](auto aliveCallback) { aliveCallback->callback(); });

    EXPECT_CALL(*callback, callback()).Times(0);
    callback.reset();
    EXPECT_NO_THROW(wrapped());
}

// Tests for wrap_weak_this
struct Subject : public std::enable_shared_from_this<Subject> {
    auto method()
    {
        auto wrapped = asyncly::wrap_weak_this(
            this, [](auto self, int a, int b) { return self->add(a, b); });

        return wrapped;
    }

    auto methodIgnore()
    {
        auto wrapped = asyncly::wrap_weak_this_ignore(
            this, [](auto self, int a, int b) { self->add(a, b); });

        return wrapped;
    }

    int add(int a, int b)
    {
        return a + b;
    }
};

TYPED_TEST(WrapTest, shouldWeakWrapThis)
{
    auto subject = std::make_shared<Subject>();

    EXPECT_EQ(3, subject->method()(1, 2));
}

TYPED_TEST(WrapTest, shouldThrowForExpiredWeakWrapThis)
{
    auto subject = std::make_shared<Subject>();
    auto wrapped = subject->method();
    subject.reset();
    EXPECT_THROW(wrapped(1, 2), std::runtime_error);
}

TYPED_TEST(WrapTest, shouldIgnoreForExpiredWeakWrapThisIgnore)
{
    auto subject = std::make_shared<Subject>();
    auto wrapped = subject->methodIgnore();
    subject.reset();
    EXPECT_NO_THROW(wrapped(1, 2));
}

// Tests for wrap_weak_this_post
struct SubjectPost : public std::enable_shared_from_this<SubjectPost> {
    SubjectPost(std::shared_ptr<std::promise<int>> called)
        : called_(called)
    {
    }

    auto methodPostIgnore(const std::shared_ptr<IExecutor>& executor)
    {
        auto wrapped
            = asyncly::wrap_weak_this_post_ignore(executor, this, [this](auto self, int a, int b) {
                  called_->set_value(self->add(a, b));
              });

        return wrapped;
    }

    int add(int a, int b)
    {
        return a + b;
    }

    std::shared_ptr<std::promise<int>> called_;
};

TYPED_TEST(WrapTest, shouldWeakWrapThisPost)
{
    auto called = std::make_shared<std::promise<int>>();

    auto subject = std::make_shared<SubjectPost>(called);
    auto wrapped = subject->methodPostIgnore(this->executor_);
    wrapped(1, 2);
    EXPECT_EQ(called->get_future().get(), 3);
}

TYPED_TEST(WrapTest, shouldNotCallExpiredWeakWrapThisPost)
{
    auto called = std::make_shared<std::promise<int>>();

    auto subject = std::make_shared<SubjectPost>(called);
    auto wrapped = subject->methodPostIgnore(this->executor_);
    subject.reset();
    wrapped(1, 2);
    this->executor_->post([called]() { called->set_value(5); });
    EXPECT_EQ(called->get_future().get(), 5);
}

TYPED_TEST(WrapTest, shouldWorkForMutableLambas)
{
    // this test verifies that this code compiles
    auto subject = std::make_shared<Subject>();
    asyncly::wrap_weak_this_ignore(subject.get(), [subject](auto) mutable { subject.reset(); });
}

TYPED_TEST(WrapTest, shouldWorkForMutableLambasWhenPosting)
{
    // this test verifies that this code compiles
    auto subject = std::make_shared<Subject>();
    asyncly::wrap_post(this->executor_, [subject]() mutable { subject.reset(); });
}
}
