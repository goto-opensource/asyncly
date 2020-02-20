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

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnull-dereference"
#endif // __clang__
#include "gmock/gmock.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__

#include "asyncly/test/TimeoutGuardEnvironment.h"
#include "asyncly/test/WindowsTimerResolution.h"

int main(int argc, char* argv[])
{
    asyncly::test::WindowsTimerResolution winTimerResolution;

    const int timeoutInSeconds = 120;

    ::testing::InitGoogleMock(&argc, argv);
    ::testing::GTEST_FLAG(catch_exceptions) = false;
    const int repeats = testing::GTEST_FLAG(repeat);
    ::testing::AddGlobalTestEnvironment(
        new asyncly::test::TimeoutGuardEnviroment(repeats * timeoutInSeconds));

    return RUN_ALL_TESTS();
}
