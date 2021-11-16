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

#include "asyncly/executor/AsioExecutorController.h"

namespace asyncly {
AsioExecutorController::AsioExecutorController(
    const ThreadConfig& threadConfig, const ISchedulerPtr& optionalScheduler)
{
    auto scheduler = optionalScheduler;
    if (!scheduler) {
        m_schedulerThread = std::make_shared<SchedulerThread>(
            threadConfig.schedulerInitFunction, std::make_shared<DefaultScheduler>());
        scheduler = m_schedulerThread->get_scheduler();
    }
    m_executor = std::make_unique<AsioExecutor>(scheduler);
    m_workerThread = std::thread([this, executorThreadConfig{ threadConfig.executorInitFunction }] {
        this_thread::set_current_executor(m_executor);
        if (executorThreadConfig) {
            executorThreadConfig();
        };
        m_executor->run();
    });
}

std::unique_ptr<asyncly::AsioExecutorController>
AsioExecutorController::create(const ThreadConfig& threadConfig, const ISchedulerPtr& scheduler)
{
    return std::unique_ptr<AsioExecutorController>(
        new AsioExecutorController(threadConfig, scheduler));
}

AsioExecutorController::~AsioExecutorController()
{
    finish();
}

void AsioExecutorController::finish()
{
    std::lock_guard<std::mutex> lock(m_stopMutex);

    if (m_schedulerThread) {
        m_schedulerThread->finish();
        m_schedulerThread.reset();
    }

    if (m_executor) {
        m_executor->finish();
    }
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

asyncly::IExecutorPtr AsioExecutorController::get_executor() const
{
    return m_executor;
}
std::shared_ptr<asyncly::IScheduler> AsioExecutorController::get_scheduler() const
{
    return m_executor->get_scheduler();
}

boost::asio::io_context& AsioExecutorController::get_io_context()
{
    return m_executor->get_io_context();
}

} // namespace asyncly