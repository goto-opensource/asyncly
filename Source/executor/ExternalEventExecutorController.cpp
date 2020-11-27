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

#include "asyncly/executor/ExternalEventExecutorController.h"

#include "asyncly/scheduler/DefaultScheduler.h"
#include "asyncly/scheduler/SchedulerThread.h"

namespace asyncly {

ExternalEventExecutorController::ExternalEventExecutorController(
    const ExternalEventFunction& externalEventFunction,
    const ThreadInitFunction& schedulerInitFunction,
    const ISchedulerPtr& optionalScheduler)
{
    auto scheduler = optionalScheduler;
    if (!scheduler) {
        m_schedulerThread = std::make_shared<SchedulerThread>(
            schedulerInitFunction, std::make_shared<DefaultScheduler>());
        scheduler = m_schedulerThread->get_scheduler();
    }

    m_executor = ExternalEventExecutor::create(scheduler, externalEventFunction);
}

ExternalEventExecutorController::~ExternalEventExecutorController()
{
    finish();
}

void ExternalEventExecutorController::runOnce()
{
    m_executor->runOnce();
}

void ExternalEventExecutorController::finish()
{
    std::lock_guard<std::mutex> lock(m_stopMutex);

    if (m_schedulerThread) {
        m_schedulerThread->finish();
        m_schedulerThread.reset();
    }
}

asyncly::IExecutorPtr ExternalEventExecutorController::get_executor() const
{
    return m_executor;
}

std::shared_ptr<asyncly::IScheduler> ExternalEventExecutorController::get_scheduler() const
{
    return m_executor->get_scheduler();
}

std::unique_ptr<ExternalEventExecutorController> ExternalEventExecutorController::create(
    const ExternalEventFunction& externalEventFunction,
    const ThreadInitFunction& schedulerInitFunction,
    const ISchedulerPtr& scheduler)
{
    auto executor
        = std::unique_ptr<ExternalEventExecutorController>(new ExternalEventExecutorController(
            externalEventFunction, schedulerInitFunction, scheduler));
    return executor;
}

}