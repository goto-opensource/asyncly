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
#include "detail/ThrowingExecutor.h"

namespace asyncly {

using namespace testing;

using ExecutorFactoryTypes = ::testing::
    Types<asyncly::test::AsioExecutorFactory<>, asyncly::test::DefaultExecutorFactory<>>;

template <typename TExecutorFactory> class WrapPostTest : public virtual Test {
  public:
    WrapPostTest()
        : executor_(factory_.create())
    {
    }

    TExecutorFactory factory_;
    std::shared_ptr<IExecutor> executor_;
};

TYPED_TEST_SUITE(WrapPostTest, ExecutorFactoryTypes);

// wrap_post
TYPED_TEST(WrapPostTest, shouldWrapPost)
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

TYPED_TEST(WrapPostTest, shouldWrapPostWithArguments)
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

TYPED_TEST(WrapPostTest, shouldWorkForMutableLambasWhenPosting)
{
    // this test verifies that this code compiles
    int i = 0;
    asyncly::wrap_post(this->executor_, [i]() mutable { i = 1; });
}

struct Callback : public std::enable_shared_from_this<Callback> {
    MOCK_METHOD0(callback, void());
    MOCK_METHOD1(callbackWithReturn, int(int));
};

class WrapWeakTest : public virtual Test {
  public:
    WrapWeakTest()
        : callback_(std::make_shared<Callback>())
    {
    }

    std::shared_ptr<Callback> callback_;
};

// wrap_weak
TEST_F(WrapWeakTest, shouldWeakWrap)
{
    auto wrapped
        = asyncly::wrap_weak(callback_, [](auto aliveCallback) { aliveCallback->callback(); });

    EXPECT_CALL(*callback_, callback());
    wrapped();
}

TEST_F(WrapWeakTest, shouldWeakWrapWithReturnValues)
{
    EXPECT_CALL(*callback_, callbackWithReturn(_)).WillOnce(ReturnArg<0>());

    auto wrapped = asyncly::wrap_weak(
        callback_, [](auto aliveCallback, int a) { return aliveCallback->callbackWithReturn(a); });

    EXPECT_EQ(12, wrapped(12));
}

TEST_F(WrapWeakTest, shouldErrorOnDeadWeakWrap)
{
    auto wrapped
        = asyncly::wrap_weak(callback_, [](auto aliveCallback) { aliveCallback->callback(); });

    EXPECT_CALL(*callback_, callback()).Times(0);
    callback_.reset();
    EXPECT_THROW(wrapped(), std::runtime_error);
}

// wrap_weak_with_custom_error
TEST_F(WrapWeakTest, shouldCustomErrorOnDeadWeakWrap)
{
    struct MyError : public std::exception { };

    auto wrapped = asyncly::wrap_weak_with_custom_error(
        callback_,
        [](auto aliveCallback) { aliveCallback->callback(); },
        []() { throw MyError{}; });

    EXPECT_CALL(*callback_, callback()).Times(0);
    callback_.reset();
    EXPECT_THROW(wrapped(), MyError);
}

// wrap_weak_ignore
TEST_F(WrapWeakTest, shouldIgnoreOnDeadWeakWrapIgnore)
{
    auto wrapped = asyncly::wrap_weak_ignore(
        callback_, [](auto aliveCallback) { aliveCallback->callback(); });

    EXPECT_CALL(*callback_, callback()).Times(0);
    callback_.reset();
    EXPECT_NO_THROW(wrapped());
}

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

class WrapWeakThisTest : public virtual Test {
  public:
    WrapWeakThisTest()
        : subject_(std::make_shared<Subject>())
    {
    }

    std::shared_ptr<Subject> subject_;
};

// wrap_weak_this
TEST_F(WrapWeakThisTest, shouldWeakWrapThis)
{
    EXPECT_EQ(3, subject_->method()(1, 2));
}

TEST_F(WrapWeakThisTest, shouldThrowForExpiredWeakWrapThis)
{
    auto wrapped = subject_->method();
    subject_.reset();
    EXPECT_THROW(wrapped(1, 2), std::runtime_error);
}

TEST_F(WrapWeakThisTest, shouldIgnoreForExpiredWeakWrapThisIgnore)
{
    auto wrapped = subject_->methodIgnore();
    subject_.reset();
    EXPECT_NO_THROW(wrapped(1, 2));
}

// wrap_weak_this_ignore
TEST_F(WrapWeakThisTest, shouldWorkForMutableLambas)
{
    // this test verifies that this code compiles
    int i = 0;
    asyncly::wrap_weak_this_ignore(subject_.get(), [i]() mutable { i = 1; });
}

template <typename TExecutorFactory>
class WrapWeakPostTest : public WrapWeakTest, public WrapPostTest<TExecutorFactory> {
};

TYPED_TEST_SUITE(WrapWeakPostTest, ExecutorFactoryTypes);

// wrap_weak_post_with_custom_error
TYPED_TEST(WrapWeakPostTest, shouldCustomErrorWithExpiredWeakWrapPost)
{
    std::promise<void> errored;

    EXPECT_CALL(*this->callback_, callback()).Times(0);

    auto wrapped = asyncly::wrap_weak_post_with_custom_error(
        this->executor_,
        this->callback_,
        [](const std::shared_ptr<Callback>& cb) { cb->callback(); },
        [&errored]() { errored.set_value(); });

    this->callback_.reset();
    wrapped();
    errored.get_future().get();
}

TYPED_TEST(WrapWeakPostTest, shouldCustomErrorWithThrowingExecutorWeakWrapPost)
{
    std::promise<void> errored;
    auto executor = asyncly::detail::ThrowingExecutor<>::create();

    EXPECT_CALL(*this->callback_, callback()).Times(0);

    auto wrapped = asyncly::wrap_weak_post_with_custom_error(
        executor,
        this->callback_,
        [](const std::shared_ptr<Callback>& cb) { cb->callback(); },
        [&errored]() { errored.set_value(); });

    wrapped();
    errored.get_future().get();
}

// wrap_weak_post_ignore
TYPED_TEST(WrapWeakPostTest, shouldWeakWrapPost)
{
    std::promise<void> done;

    EXPECT_CALL(*this->callback_, callback()).Times(0);

    auto wrapped = asyncly::wrap_weak_post_ignore(
        this->executor_, this->callback_, [&done](const std::shared_ptr<Callback>& cb) {
            cb->callback();
            done.set_value();
        });

    Mock::VerifyAndClearExpectations(this->callback_.get());

    EXPECT_CALL(*this->callback_, callback());
    wrapped();
    done.get_future().get();
}

TYPED_TEST(WrapWeakPostTest, shouldWeakWrapPostWithArguments)
{
    std::promise<int> done;
    int x = 1;
    int y = 2;

    auto wrapped = asyncly::wrap_weak_post_ignore(
        this->executor_,
        this->callback_,
        [&done](const std::shared_ptr<Callback>& cb, int a, int b) {
            cb->callback();
            done.set_value(a + b);
        });

    Mock::VerifyAndClearExpectations(this->callback_.get());

    EXPECT_CALL(*this->callback_, callback());
    wrapped(x, y);
    EXPECT_EQ(done.get_future().get(), x + y);
}

TYPED_TEST(WrapWeakPostTest, shouldNotCallExpiredWeakWrapPost)
{
    std::promise<void> done;

    EXPECT_CALL(*this->callback_, callback()).Times(0);

    auto wrapped = asyncly::wrap_weak_post_ignore(
        this->executor_, this->callback_, [](const std::shared_ptr<Callback>& cb) {
            cb->callback();
        });

    this->callback_.reset();
    wrapped();
    this->executor_->post([&done]() { done.set_value(); });
    done.get_future().get();
}

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

template <typename TExecutorFactory>
class WrapWeakThisPostTest : public WrapPostTest<TExecutorFactory> {
  public:
    WrapWeakThisPostTest()
        : called_(std::make_shared<std::promise<int>>())
        , subject_(std::make_shared<SubjectPost>(called_))
    {
    }

    std::shared_ptr<std::promise<int>> called_;
    std::shared_ptr<SubjectPost> subject_;
};

TYPED_TEST_SUITE(WrapWeakThisPostTest, ExecutorFactoryTypes);

// wrap_weak_this_post_ignore
TYPED_TEST(WrapWeakThisPostTest, shouldWeakWrapThisPost)
{
    auto wrapped = this->subject_->methodPostIgnore(this->executor_);
    wrapped(1, 2);
    EXPECT_EQ(this->called_->get_future().get(), 3);
}

TYPED_TEST(WrapWeakThisPostTest, shouldNotCallExpiredWeakWrapThisPost)
{
    auto wrapped = this->subject_->methodPostIgnore(this->executor_);
    this->subject_.reset();
    wrapped(1, 2);
    this->executor_->post([called{ this->called_ }]() { called->set_value(5); });
    EXPECT_EQ(this->called_->get_future().get(), 5);
}
} // namespace asyncly
