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

#include <benchmark/benchmark.h>

#include "executor/detail/StrandImpl.h"
#include <asyncly/executor/ExceptionShield.h>
#include <asyncly/executor/MetricsWrapper.h>
#include <asyncly/executor/Strand.h>
#include <asyncly/executor/ThreadPoolExecutorController.h>

namespace asyncly {
static const size_t kBatchSize = 10000;

static void testExecutor(benchmark::State& state, const IExecutorPtr& executor)
{
    size_t iterations = 0;
    for (const auto _ : state) {
        std::promise<void> done;
        for (size_t i = 1; i <= kBatchSize; i++) {
            executor->post([i, &done]() {
                if (i == kBatchSize) {
                    done.set_value();
                }
            });
        }
        done.get_future().wait();
        iterations++;
    }
    state.SetItemsProcessed(iterations * kBatchSize);
}
}

static void executorThreadPoolTest(benchmark::State& state)
{
    auto executor_ = asyncly::ThreadPoolExecutorController::create(1);

    asyncly::testExecutor(state, executor_->get_executor());
}

static void metricsWrapperTest(benchmark::State& state)
{
    auto executorControl = asyncly::ThreadPoolExecutorController::create(1);
    auto executor_ = asyncly::MetricsWrapper<>::create(executorControl->get_executor());

    asyncly::testExecutor(state, executor_);
}

static void exceptionShieldTest(benchmark::State& state)
{
    auto executorControl = asyncly::ThreadPoolExecutorController::create(1);
    auto executor_ = asyncly::ExceptionShield::create(
        executorControl->get_executor(), [](std::exception_ptr) {});

    asyncly::testExecutor(state, executor_);
}

static void strandTest(benchmark::State& state)
{
    auto executorControl = asyncly::ThreadPoolExecutorController::create(1);
    auto executor_ = std::make_shared<asyncly::StrandImpl>(executorControl->get_executor());

    asyncly::testExecutor(state, executor_);
}

BENCHMARK(executorThreadPoolTest);
BENCHMARK(metricsWrapperTest);
BENCHMARK(exceptionShieldTest);
BENCHMARK(strandTest);
