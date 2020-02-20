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

#include "gmock/gmock.h"

#include <chrono>
#include <future>
#include <mutex>
#include <thread>

#include "asyncly/executor/CurrentExecutor.h"
#include "asyncly/executor/ThreadPoolExecutorController.h"

namespace asyncly {

using namespace testing;

class ExecutorThreadPoolTest : public Test {
  public:
    ExecutorThreadPoolTest()
        : executor_(ThreadPoolExecutorController::create(1))
    {
        asyncly::this_thread::set_current_executor(IExecutorPtr());
    }

    std::shared_ptr<ThreadPoolExecutorController> executor_;
};

TEST_F(ExecutorThreadPoolTest, shouldInitializeWithOneThread)
{
    auto executor = ThreadPoolExecutorController::create(1);
}

TEST_F(ExecutorThreadPoolTest, shouldInitializeWithMultipleThreads)
{
    auto executor = ThreadPoolExecutorController::create(10);
}

}
