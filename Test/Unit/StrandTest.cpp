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

#include "asyncly/executor/ExceptionShield.h"
#include "asyncly/executor/Strand.h"
#include "asyncly/executor/ThreadPoolExecutorController.h"
#include "asyncly/test/FakeExecutor.h"
#include "executor/detail/StrandImpl.h"

using namespace asyncly;
using namespace testing;

class StrandTest : public Test {
  public:
    StrandTest()
        : fakeExecutor_{ asyncly::test::FakeExecutor::create() }
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

class CreateStrandTest : public Test {
};

TEST_F(CreateStrandTest, shouldCreateStrandIfNonSerializedExecutor)
{
    auto controller = ThreadPoolExecutorController::create(2);
    auto executor = controller->get_executor();
    auto strand = asyncly::create_strand(executor);
    ASSERT_NE(strand, executor);
    ASSERT_TRUE(std::dynamic_pointer_cast<StrandImpl>(strand) != nullptr);
}

TEST_F(CreateStrandTest, shouldPassThroughWhenStaticTypeIsAlreadyStrand)
{
    auto controller = ThreadPoolExecutorController::create(2);
    auto executor = controller->get_executor();
    auto strand = asyncly::create_strand(executor);
    auto strand2 = asyncly::create_strand(strand);
    ASSERT_EQ(strand, strand2);
}

TEST_F(CreateStrandTest, shouldPassThroughWhenDynamicTypeIsAlreadyStrand)
{
    auto controller = ThreadPoolExecutorController::create(1);
    auto executor = controller->get_executor();
    asyncly::IExecutorPtr strand = asyncly::create_strand(executor);
    ASSERT_EQ(strand, executor);
}
