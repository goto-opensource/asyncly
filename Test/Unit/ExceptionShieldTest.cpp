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
#include <memory>

#include "gmock/gmock.h"

#include "asyncly/executor/ExceptionShield.h"
#include "asyncly/executor/ThreadPoolExecutorController.h"

using namespace testing;
using namespace asyncly;

class ExceptionShieldTest : public Test {
  public:
    void SetUp() override
    {
        executorController_ = ThreadPoolExecutorController::create(1);
        executor_ = executorController_->get_executor();
    }

    std::shared_ptr<IExecutorController> executorController_;
    std::shared_ptr<IExecutor> executor_;
};

TEST_F(ExceptionShieldTest, shouldRunPostedTask)
{
    std::promise<void> taskIsRun;
    auto exceptionShield = create_exception_shield(executor_, [](std::exception_ptr) {});
    exceptionShield->post([&taskIsRun]() { taskIsRun.set_value(); });

    taskIsRun.get_future().wait();
}

TEST_F(ExceptionShieldTest, shouldCallExceptionHandler)
{
    std::promise<void> exceptionIsThrown;
    auto exceptionHandler = [&exceptionIsThrown](auto) { exceptionIsThrown.set_value(); };
    auto exceptionShield = create_exception_shield(executor_, exceptionHandler);
    exceptionShield->post([]() { throw std::runtime_error(""); });
    exceptionIsThrown.get_future().wait();
}

TEST_F(ExceptionShieldTest, shouldCaptureThrownIntegers)
{
    std::promise<int> thrownInteger;
    int integerToBeThrown = 42;
    auto exceptionHandler = [&thrownInteger](std::exception_ptr e) {
        try {
            std::rethrow_exception(e);
        } catch (int i) {
            thrownInteger.set_value(i);
        }
    };
    auto exceptionShield = create_exception_shield(executor_, exceptionHandler);
    exceptionShield->post([integerToBeThrown]() { throw int{ integerToBeThrown }; });
    EXPECT_EQ(integerToBeThrown, thrownInteger.get_future().get());
}

TEST_F(ExceptionShieldTest, shouldNotAcceptInvalidExecutor)
{
    auto createShield = []() { auto e = create_exception_shield({}, [](std::exception_ptr) {}); };
    EXPECT_THROW(createShield(), std::exception);
}

TEST_F(ExceptionShieldTest, shouldNotAcceptNullExceptionHandlers)
{
    auto createShield = [this]() { auto e = create_exception_shield(this->executor_, {}); };
    EXPECT_THROW(createShield(), std::exception);
}
