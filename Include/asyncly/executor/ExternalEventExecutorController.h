/*
 * Copyright 2020 LogMeIn
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

#include <mutex>

#include "asyncly/executor/detail/ExternalEventExecutor.h"

#include "asyncly/executor/IExecutorController.h"

#include "asyncly/ExecutorTypes.h"

namespace asyncly {

/**
 * ExternalEventExecutor can be used to integrate with an existing thread and event loop.
 * Example: There is a network event handling based on epoll which requires a thread waiting
 * synchronously on epoll_wait. If you want to have an executor in the same thread context (to
 * expose proxies), you have to integrate with the epoll event loop. ExternalEventExecutor allows to
 * pass an externalEventFunction parameter which is called by the executor when tasks are posted.
 * This function should "wake up" the external event loop and call then
 * ExternalEventExecutorController::runOnce() in the context of the event loop thread.
 */
class ExternalEventExecutorController final : public IExecutorController {
  public:
    static std::unique_ptr<ExternalEventExecutorController> create(
        const ExternalEventFunction& externalEventFunction,
        const ThreadInitFunction& schedulerInitFunction,
        const ISchedulerPtr& scheduler = {});

    ~ExternalEventExecutorController();

    ExternalEventExecutorController(ExternalEventExecutorController const&) = delete;
    ExternalEventExecutorController& operator=(ExternalEventExecutorController const&) = delete;

    void runOnce();

    void finish() override;
    IExecutorPtr get_executor() const override;
    std::shared_ptr<IScheduler> get_scheduler() const override;

  private:
    ExternalEventExecutorController(
        const ExternalEventFunction& externalEventFunction,
        const ThreadInitFunction& schedulerInitFunction,
        const ISchedulerPtr& scheduler);

  private:
    std::shared_ptr<ExternalEventExecutor> m_executor;
    std::shared_ptr<SchedulerThread> m_schedulerThread;
    std::mutex m_stopMutex;
};

}
