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

#include <deque>

#include "gmock/gmock.h"

#include "asyncly/executor/Strand.h"
#include "asyncly/executor/ThreadPoolExecutorController.h"
#include "asyncly/test/FakeExecutor.h"
#include "executor/detail/StrandImpl.h"

using namespace asyncly;
using namespace testing;

class StrandTest : public Test {
  public:
    StrandTest()
        : fakeExecutor_{ std::make_shared<asyncly::test::FakeExecutor>() }
        , strand_{ std::make_shared<StrandImpl>(fakeExecutor_) }
    {
    }

    std::shared_ptr<asyncly::test::FakeExecutor> fakeExecutor_;
    std::shared_ptr<IExecutor> strand_;
};

TEST_F(StrandTest, shouldPostImmediately)
{
    auto run = false;
    strand_->post([&run]() { run = true; });
    fakeExecutor_->runTasks();
    EXPECT_TRUE(run);
}

TEST_F(StrandTest, shouldSerializeExecution)
{
    auto run1 = false;
    auto run2 = false;
    strand_->post([&run1]() { run1 = true; });
    strand_->post([&run2]() { run2 = true; });

    auto secondTaskHasNotBeenPostedToExecutor = fakeExecutor_->queuedTasks() == 1;
    EXPECT_TRUE(secondTaskHasNotBeenPostedToExecutor);

    fakeExecutor_->runTasks(1U);
    auto secondTaskHasBeenPostedToExecutor = fakeExecutor_->queuedTasks() == 1;
    EXPECT_TRUE(secondTaskHasBeenPostedToExecutor);

    EXPECT_TRUE(run1);
    EXPECT_FALSE(run2);

    fakeExecutor_->runTasks(1U);

    EXPECT_TRUE(run2);
}

class StrandFactoryTest : public Test {
};

TEST_F(StrandFactoryTest, shouldCreateStrandIfNonserializedExecutor)
{
    auto multiThreadPoolController = ThreadPoolExecutorController::create(2);
    auto multiThreadExecutor = multiThreadPoolController->get_executor();
    ASSERT_FALSE(multiThreadExecutor->is_serializing());
    auto strand = asyncly::create_strand(multiThreadExecutor);
    ASSERT_TRUE(strand->is_serializing());
    ASSERT_NE(strand, multiThreadExecutor);
}

TEST_F(StrandFactoryTest, shouldNotCreateStrandIfSerializedExecutor)
{
    auto multiThreadPoolController = ThreadPoolExecutorController::create(1);
    auto multiThreadExecutor = multiThreadPoolController->get_executor();
    ASSERT_TRUE(multiThreadExecutor->is_serializing());
    auto strand = asyncly::create_strand(multiThreadExecutor);
    ASSERT_TRUE(strand->is_serializing());
    ASSERT_EQ(strand, multiThreadExecutor);
}
