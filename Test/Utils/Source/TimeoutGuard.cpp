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

#include "TimeoutGuard.h"

#include <cstdlib>
#include <iostream>

namespace asyncly {
namespace test {

TimeoutGuard::TimeoutGuard(std::time_t timeoutInSec)
    : timeout_{ boost::chrono::seconds(timeoutInSec) }
    , testDone_(false)
{
}

TimeoutGuard::~TimeoutGuard()
{
    stop();
}

/// start running the background thread which kill the application on timeout
void TimeoutGuard::start()
{
    stop();

    testDone_ = false;

    // We spawn a background thread that calls abort() when the timeout is reached
    thread_ = std::thread([&]() { worker(); });
}

/// stop the background thread
void TimeoutGuard::stop()
{
    testDone_ = true;
    conditionVariable_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void TimeoutGuard::worker()
{
    boost::unique_lock<boost::mutex> lock{ mutex_ };
    auto notTimedOut = conditionVariable_.wait_for(lock, timeout_, [this]() { return testDone_; });

    if (!notTimedOut) {
        std::cerr << std::endl << "ERROR: Test Guard timed out" << std::endl;
        std::abort();
    }
}
}
} // namespace asyncly::test
