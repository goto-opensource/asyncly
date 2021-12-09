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

#include "asyncly/proxy/ProxyAssertions.h"
#include "asyncly/ProxyFactory.h"
#include "asyncly/test/FakeExecutor.h"

#include "InterfaceForExecutorTest.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include <stdexcept>

namespace asyncly {

using namespace testing;

class MockInterfaceForExecutorTest : public InterfaceForExecutorTest {
  public:
    MOCK_METHOD0(method, void());
    MOCK_METHOD2(methodWithArguments, void(std::unique_ptr<bool> a, int c));
    MOCK_METHOD1(methodReturningFuture, Future<int>(bool b));
    MOCK_METHOD1(methodReturningVoidFuture, Future<void>(bool b));
    MOCK_METHOD1(methodReturningObservable, Observable<int>(bool b));
};

class ProxyAssertionsTest : public Test {
  public:
    ProxyAssertionsTest()
        : fakeExecutor_{ asyncly::test::FakeExecutor::create() }
        , interfaceMock_(std::make_shared<MockInterfaceForExecutorTest>())
    {
    }

    std::shared_ptr<asyncly::test::FakeExecutor> fakeExecutor_;
    std::shared_ptr<MockInterfaceForExecutorTest> interfaceMock_;
};

TEST_F(ProxyAssertionsTest, proxyCheckSuccess)
{
    std::shared_ptr<InterfaceForExecutorTest> proxy
        = asyncly::ProxyFactory<InterfaceForExecutorTest>::createWeakProxy(
            interfaceMock_, fakeExecutor_);
    EXPECT_NO_THROW(asyncly::check_proxy(proxy));
}

TEST_F(ProxyAssertionsTest, proxyCheckFailure)
{
    EXPECT_THROW(asyncly::check_proxy(interfaceMock_), std::runtime_error);
}

} // namespace asyncly
