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
    for (const auto _ : state) {
        std::promise<void> done;
        for (size_t i = 1; i <= kBatchSize; i++) {
            executor->post([i, &done]() {
                if (i == kBatchSize) {
                    done.set_value();
                }
            });
        }
        done.get_future().get();
    }
    state.SetItemsProcessed(state.iterations() * kBatchSize);
}
}

static void executorThreadPoolTest(benchmark::State& state)
{
    auto executorController = asyncly::ThreadPoolExecutorController::create(1);
    auto executor = executorController->get_executor();

    asyncly::testExecutor(state, executor);
}

static void metricsWrapperTest(benchmark::State& state)
{
    auto executorController = asyncly::ThreadPoolExecutorController::create(1);
    auto executor = asyncly::create_metrics_wrapper(executorController->get_executor(), "");

    asyncly::testExecutor(state, executor.first);
}

static void exceptionShieldTest(benchmark::State& state)
{
    auto executorController = asyncly::ThreadPoolExecutorController::create(1);
    auto executor = asyncly::create_exception_shield(
        executorController->get_executor(), [](std::exception_ptr) {});

    asyncly::testExecutor(state, executor);
}

static void strandTest(benchmark::State& state)
{
    auto executorController = asyncly::ThreadPoolExecutorController::create(1);
    auto executor = std::make_shared<asyncly::StrandImpl>(executorController->get_executor());

    asyncly::testExecutor(state, executor);
}

BENCHMARK(executorThreadPoolTest);
BENCHMARK(metricsWrapperTest);
BENCHMARK(exceptionShieldTest);
BENCHMARK(strandTest);
