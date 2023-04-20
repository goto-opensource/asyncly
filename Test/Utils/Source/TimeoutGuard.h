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
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <thread>

#include <boost/noncopyable.hpp>

namespace asyncly::test {

/// Allows to kill an application after a certain timeout
class TimeoutGuard : boost::noncopyable {
  public:
    TimeoutGuard(std::time_t timeoutInSec);

    ~TimeoutGuard();

    /// start running the background thread which kill the application on timeout
    void start();

    /// stop the background thread
    void stop();

  private:
    void worker();

    using clock = std::chrono::steady_clock;

    const clock::duration timeout_;
    std::thread thread_;
    bool testDone_;
    std::mutex mutex_;
    std::condition_variable conditionVariable_;
};
} // namespace asyncly::test
