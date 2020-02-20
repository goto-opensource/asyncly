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
#include <iostream>
#include <thread>

#include <boost/chrono.hpp>
#include <boost/thread.hpp>

#include <benchmark/benchmark.h>

#include <asyncly/executor/ThreadPoolExecutorController.h>

namespace asyncly {
namespace {
const uint64_t counter_increment = 10000;
std::atomic<bool> keepRunning{ true };
std::atomic<uint64_t> result{ 0 };
thread_local uint64_t counter{ 0 };

static void
send(std::shared_ptr<IExecutor> executorToCallIn, std::shared_ptr<IExecutor> executorToCallBack)
{
    if (!keepRunning.load()) {
        return;
    }

    auto task
        = [executorToCallIn, executorToCallBack]() { send(executorToCallBack, executorToCallIn); };
    executorToCallIn->post(task);

    counter++;
    if (counter % counter_increment == 0) {
        result.fetch_add(counter_increment, std::memory_order_relaxed);
    }
}
}
}

using namespace asyncly;

static void threadPoolPerformanceTest(benchmark::State& state)
{
    keepRunning = true;

    auto range = static_cast<size_t>(state.range(0));

    auto executorControlA = ThreadPoolExecutorController::create(range);
    auto executorA = executorControlA->get_executor();

    auto executorControlB = ThreadPoolExecutorController::create(range);
    auto executorB = executorControlB->get_executor();

    auto lastTs = std::chrono::steady_clock::now();

    for (unsigned i = 0; i < 10 * range; i++) {
        send(executorB, executorA);
    }

    bool allowedToRun = true;
    uint64_t lastResult = 0;
    while (allowedToRun) {
        uint64_t currentResult = result;
        while (lastResult == currentResult) {
            boost::this_thread::sleep_for(boost::chrono::microseconds(5));
            currentResult = result;
        }
        const auto afterTs = std::chrono::steady_clock::now();

        auto incrementCount = (currentResult - lastResult);
        const auto iterationTime
            = std::chrono::duration_cast<std::chrono::duration<double>>(afterTs - lastTs).count()
            / incrementCount;

        for (size_t i = 0; allowedToRun && i < incrementCount; ++i) {
            allowedToRun = state.KeepRunning();
            state.SetIterationTime(iterationTime);
        }

        lastTs = afterTs;
        lastResult = currentResult;
    }
    keepRunning = false;
    state.SetItemsProcessed(result.exchange(0));

    std::promise<void> doneA;
    executorA->post([&doneA]() { doneA.set_value(); });
    std::promise<void> doneB;
    executorB->post([&doneB]() { doneB.set_value(); });
    doneA.get_future().wait();
    doneB.get_future().wait();
}

BENCHMARK(threadPoolPerformanceTest)
    ->Arg(1)
    ->Arg(2)
    ->Arg(std::thread::hardware_concurrency() / 2)
    ->UseManualTime();