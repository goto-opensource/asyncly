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

#pragma once

#include <chrono>
#include <thread>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace asyncly {
namespace this_thread {

/*! Implements platform specific sleep_for.
 *
 * This function is intended as a replacement for std::this_thread::sleep_for()
 * which is currently (VS2017) broken on Windows because it is using the system
 * clock instead of a monotonic one.
 *
 * On Windows the function is implemented using Win32 primitives, on all other
 * platforms it just wraps std::this_thread::sleep_for().
 * It only gives the guarantee that it will not return before the desired
 * sleep duration, sub-millisecond accuracy will not be achieved.
 *
 * \param sleep_duration    The amount of time to suspend the current thread.
 */
template <class Rep, class Period>
void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration)
{
#if defined(_WIN32)
    auto before = std::chrono::steady_clock::now();
    auto slept = std::chrono::nanoseconds{ 0 };
    while (slept < sleep_duration) {
        auto duration_ms = std::chrono::ceil<std::chrono::milliseconds>(sleep_duration - slept);
        ::Sleep(static_cast<DWORD>(duration_ms.count()));
        slept = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - before);
    }
#else
    std::this_thread::sleep_for(sleep_duration);
#endif
}
}
}