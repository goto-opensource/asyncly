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

#include "asyncly/executor/ThreadPoolExecutorController.h"

namespace asyncly {

ThreadPoolExecutorController::ThreadPoolExecutorController(
    const ThreadPoolConfig& threadPoolConfig, const ISchedulerPtr& optionalScheduler)
{
    auto scheduler = optionalScheduler;
    if (!scheduler) {
        m_schedulerThread = std::make_shared<SchedulerThread>(
            threadPoolConfig.schedulerInitFunction, std::make_shared<DefaultScheduler>());
        scheduler = m_schedulerThread->get_scheduler();
    }

    m_executor = ThreadPoolExecutor::create(scheduler);

    for (const auto& threadInitFunction : threadPoolConfig.executorInitFunctions) {
        m_workerThreads.emplace_back([this, threadInitFunction]() {
            if (threadInitFunction) {
                threadInitFunction();
            }
            m_executor->run();
        });
    }
}

ThreadPoolExecutorController::~ThreadPoolExecutorController()
{
    finish();
}

void ThreadPoolExecutorController::finish()
{
    std::lock_guard<std::mutex> lock(m_stopMutex);

    if (m_schedulerThread) {
        m_schedulerThread->finish();
        m_schedulerThread.reset();
    }

    m_executor->finish();
    for (auto& thread : m_workerThreads) {
        thread.join();
    }
    m_workerThreads.clear();
}

asyncly::IExecutorPtr ThreadPoolExecutorController::get_executor() const
{
    return m_executor;
}

std::shared_ptr<asyncly::IScheduler> ThreadPoolExecutorController::get_scheduler() const
{
    return m_executor->get_scheduler();
}

std::unique_ptr<asyncly::ThreadPoolExecutorController>
ThreadPoolExecutorController::create(size_t numberOfThreads, const ISchedulerPtr& scheduler)
{
    ThreadPoolConfig threadPoolConfig;
    for (auto i = 0u; i < numberOfThreads; ++i) {
        threadPoolConfig.executorInitFunctions.emplace_back([]() {});
    }
    return create(threadPoolConfig, scheduler);
}

std::unique_ptr<ThreadPoolExecutorController> ThreadPoolExecutorController::create(
    const ThreadPoolConfig& threadPoolConfig, const ISchedulerPtr& scheduler)
{
    auto executor = std::unique_ptr<ThreadPoolExecutorController>(
        new ThreadPoolExecutorController(threadPoolConfig, scheduler));
    return executor;
}

}