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

#include "asyncly/test/WindowsTimerResolution.h"

#ifdef _WIN32
#pragma comment(lib, "winmm.lib")
#include <Windows.h>
#include <timeapi.h>
#endif
#include <sstream>
#include <stdexcept>

namespace asyncly {
namespace test {

WindowsTimerResolution::WindowsTimerResolution(const std::chrono::milliseconds& timerResolution)
    : timerResolution_(timerResolution)
{
#ifdef _WIN32
    auto resolution = static_cast<UINT>(timerResolution_.count());
    auto result = ::timeBeginPeriod(resolution);
    if (TIMERR_NOCANDO == result) {
        std::ostringstream ostr;
        ostr << "timeBeginPeriod failed: ";

        TIMECAPS timeCaps;
        auto devCapsResult = ::timeGetDevCaps(&timeCaps, sizeof(timeCaps));
        if (TIMERR_NOERROR == devCapsResult) {
            ostr << "requested resolution (" << resolution << "ms ) is out of supported range [ "
                 << timeCaps.wPeriodMin << "ms, " << timeCaps.wPeriodMax << "ms ]";
        }
        throw std::runtime_error(ostr.str());
    }
#else
    (void)timerResolution_;
#endif // _WIN32
}

WindowsTimerResolution::~WindowsTimerResolution()
{
#ifdef _WIN32
    auto resolution = static_cast<UINT>(timerResolution_.count());
    ::timeEndPeriod(resolution);
#endif // _WIN32
}
}
} // namespace asyncly::test
