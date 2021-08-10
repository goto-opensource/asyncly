/*
 * Copyright 2021 LogMeIn
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

#include "asyncly/future/BlockingWait.h"
#include "asyncly/executor/IExecutor.h"

#include "StrandImplTestFactory.h"
#include "asyncly/test/ExecutorTestFactories.h"

#include "gmock/gmock.h"

namespace asyncly {

using namespace testing;

template <typename TExecutorFactory> class BlockingWaitTest : public Test {
  public:
    BlockingWaitTest()
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

TYPED_TEST_SUITE(BlockingWaitTest, ExecutorFactoryTypes);

TYPED_TEST(BlockingWaitTest, shouldPostAndWaitForTaskReturningVoidFuture)
{
    std::atomic<bool> check = false;
    const auto slowTask = [&check]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        check = true;
        return make_ready_future();
    };

    blocking_wait(this->executor_, slowTask);

    EXPECT_TRUE(check);
}

TYPED_TEST(BlockingWaitTest, shouldPostAndWaitForTaskReturningNonVoidFuture)
{
    std::atomic<bool> check = false;
    const auto slowTask = [&check]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        check = true;
        return make_ready_future(true);
    };

    EXPECT_TRUE(blocking_wait(this->executor_, slowTask));

    EXPECT_TRUE(check);
}

TYPED_TEST(BlockingWaitTest, shouldWaitForVoidFuture)
{
    std::atomic<bool> check = false;
    auto promise = std::make_shared<Promise<void>>();
    this->executor_->post_after(std::chrono::milliseconds(20), [&check, promise]() {
        check = true;
        promise->set_value();
    });

    blocking_wait(promise->get_future());

    EXPECT_TRUE(check);
}

TYPED_TEST(BlockingWaitTest, shouldWaitForNonVoidFuture)
{
    std::atomic<bool> check = false;
    auto promise = std::make_shared<Promise<bool>>();
    this->executor_->post_after(std::chrono::milliseconds(20), [&check, promise]() {
        check = true;
        promise->set_value(true);
    });

    EXPECT_TRUE(blocking_wait(promise->get_future()));

    EXPECT_TRUE(check);
}

TYPED_TEST(BlockingWaitTest, shouldWaitForMultipleNonVoidFuture)
{
    std::atomic<bool> check0 = false;
    auto promise0 = std::make_shared<Promise<bool>>();
    this->executor_->post_after(std::chrono::milliseconds(20), [&check0, promise0]() {
        check0 = true;
        promise0->set_value(true);
    });

    std::atomic<bool> check1 = false;
    auto promise1 = std::make_shared<Promise<bool>>();
    this->executor_->post_after(std::chrono::milliseconds(20), [&check1, promise1]() {
        check1 = true;
        promise1->set_value(true);
    });

    auto [res0, res1] = blocking_wait_all(promise0->get_future(), promise1->get_future());

    EXPECT_TRUE(res0);
    EXPECT_TRUE(res1);
    EXPECT_TRUE(check0);
    EXPECT_TRUE(check1);
}
}