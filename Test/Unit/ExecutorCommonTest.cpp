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

#include "gtest/gtest.h"

#include "asyncly/executor/AsioExecutorController.h"
#include "asyncly/executor/ExceptionShield.h"
#include "asyncly/executor/MetricsWrapper.h"
#include "asyncly/executor/ThreadPoolExecutorController.h"
#include "asyncly/test/ExecutorInterfaceTest.h"
#include "asyncly/test/ExecutorTestFactories.h"

#include "StrandImplTestFactory.h"
#include "executor/detail/StrandImpl.h"

namespace asyncly {
namespace test {

using ExecutorFactoryTypes = ::testing::Types<
    asyncly::test::AsioExecutorFactory<>,
    asyncly::test::DefaultExecutorFactory<>,
    asyncly::test::DefaultExecutorFactory<5>,
    asyncly::test::StrandImplTestFactory<>>;

INSTANTIATE_TYPED_TEST_SUITE_P(ThreadPoolExecutor, ExecutorCommonTest, ExecutorFactoryTypes);

using ScheduledExecutorFactoryTypes = ::testing::Types<
    AsioExecutorFactory<>,
    DefaultExecutorFactory<>,
    StrandImplTestFactory<>,
    AsioExecutorFactory<SchedulerProviderDefault>,
    DefaultExecutorFactory<1, SchedulerProviderDefault>,
    StrandImplTestFactory<SchedulerProviderDefault>
    /*, disabled SchedulerProviderAsio due to flaky test
    (https://jira.ops.expertcity.com/browse/ACINI-1142)
    Executor/ScheduledExecutorCommonTest/9.shouldExecuteBeforeNewestExpires,
    where TypeParam = class asyncly::test::ProxyExecutorFactory<class
    asyncly::test::SchedulerProviderExternal<class asyncly::AsioScheduler> >

    AsioExecutorFactory<SchedulerProviderAsio>,
    DefaultExecutorFactory<1, SchedulerProviderAsio>,
    StrandImplTestFactory<SchedulerProviderAsio>
    */
    >;

INSTANTIATE_TYPED_TEST_SUITE_P(
    ThreadPoolExecutor, ScheduledExecutorCommonTest, ScheduledExecutorFactoryTypes);
}

/**
 * Test of the IExecutor::is_serializing property
 */
class SerializingPropertyTest : public ::testing::Test {
};

TEST_F(SerializingPropertyTest, singleThreadIsSerializing)
{
    auto singleThreadExecutorController = ThreadPoolExecutorController::create(1);
    ASSERT_TRUE(is_serializing(singleThreadExecutorController->get_executor()));
}

TEST_F(SerializingPropertyTest, multiThreadIsNotSerializing)
{
    auto multiThreadExecutorController = ThreadPoolExecutorController::create(2);
    ASSERT_FALSE(is_serializing(multiThreadExecutorController->get_executor()));
}

TEST_F(SerializingPropertyTest, strandIsSerializing)
{
    auto multiThreadExecutorController = ThreadPoolExecutorController::create(2);
    auto strand = std::make_shared<StrandImpl>(multiThreadExecutorController->get_executor());
    ASSERT_TRUE(is_serializing(strand));
}

TEST_F(SerializingPropertyTest, maintainPropertyTroughExceptionShield1)
{
    auto multiThreadExecutorController = ThreadPoolExecutorController::create(2);
    auto wrapper
        = create_exception_shield(multiThreadExecutorController->get_executor(), [](auto) {});
    ASSERT_FALSE(is_serializing(wrapper));
}

TEST_F(SerializingPropertyTest, maintainPropertyTroughExceptionShield2)
{
    auto singleThreadExecutorController = ThreadPoolExecutorController::create(1);
    auto wrapper
        = create_exception_shield(singleThreadExecutorController->get_executor(), [](auto) {});
    ASSERT_TRUE(is_serializing(wrapper));
}

TEST_F(SerializingPropertyTest, maintainPropertyTroughMetricsWrapper1)
{
    auto multiThreadExecutorController = ThreadPoolExecutorController::create(2);
    auto wrapper = create_metrics_wrapper(multiThreadExecutorController->get_executor(), "");
    ASSERT_FALSE(is_serializing(wrapper.first));
}

TEST_F(SerializingPropertyTest, maintainPropertyTroughMetricsWrapper2)
{
    auto singleThreadExecutorController = ThreadPoolExecutorController::create(1);
    auto wrapper = create_metrics_wrapper(singleThreadExecutorController->get_executor(), "");
    ASSERT_TRUE(is_serializing(wrapper.first));
}

TEST_F(SerializingPropertyTest, asioExecutorIsCurrentlySerializing)
{
    auto asioExecutorController = AsioExecutorController::create({}, {});
    ASSERT_TRUE(is_serializing(asioExecutorController->get_executor()));
}

}
