/*
 * Copyright 2022 GoTo
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

#include "asyncly/future/LazyValue.h"
#include "asyncly/test/FakeFutureTest.h"

#include "gmock/gmock.h"

namespace asyncly {

using namespace testing;

class LazyValueTest : public test::FakeFutureTest {
  public:
    LazyValueTest()
        : lazyValue_(std::make_shared<LazyValue<std::string>>())
    {
    }

  protected:
    std::shared_ptr<LazyValue<std::string>> lazyValue_;
};

TEST_F(LazyValueTest, resolvesFutureWhenValueIsSet)
{
    auto expectedValue = std::string{ "expected" };
    lazyValue_->set_value(expectedValue);
    auto actualValue = wait_for_future(lazyValue_->get_future());
    EXPECT_EQ(expectedValue, actualValue);
}

TEST_F(LazyValueTest, resolvesFutureAfterValueIsSet)
{
    auto expectedValue = std::string{ "expected" };
    auto future = lazyValue_->get_future();
    lazyValue_->set_value(expectedValue);
    auto actualValue = wait_for_future(std::move(future));
    EXPECT_EQ(expectedValue, actualValue);
}

TEST_F(LazyValueTest, resolvesMultipleFutures)
{
    auto expectedValue = std::string{ "expected" };
    lazyValue_->set_value(expectedValue);
    auto actualValue1 = wait_for_future(lazyValue_->get_future());
    EXPECT_EQ(expectedValue, actualValue1);
    auto actualValue2 = wait_for_future(lazyValue_->get_future());
    EXPECT_EQ(expectedValue, actualValue2);
}

TEST_F(LazyValueTest, rejectsWhenLazyValueIsResetWithNoValueSet)
{
    auto future = lazyValue_->get_future();
    lazyValue_.reset();
    wait_for_future_failure(std::move(future));
}

TEST_F(LazyValueTest, resolvesWhenLazyValueIsResetWithValueSet)
{
    auto expectedValue = std::string{ "expected" };
    lazyValue_->set_value(expectedValue);
    auto future = lazyValue_->get_future();
    lazyValue_.reset();
    auto actualValue = wait_for_future(std::move(future));
    EXPECT_EQ(expectedValue, actualValue);
}

TEST_F(LazyValueTest, hasValue)
{
    EXPECT_FALSE(lazyValue_->has_value());
    lazyValue_->set_value("someValue");
    EXPECT_TRUE(lazyValue_->has_value());
}

} // namespace asyncly