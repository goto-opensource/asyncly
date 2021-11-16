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

#include <mutex>
#include <thread>

#include "detail/ThreadPoolExecutor.h"

#include "IExecutor.h"
#include "IExecutorController.h"

#include "asyncly/ExecutorTypes.h"
#include "asyncly/scheduler/SchedulerThread.h"

namespace asyncly {

class ThreadPoolExecutorController final : public IExecutorController {
  public:
    static std::unique_ptr<ThreadPoolExecutorController>
    create(size_t numberOfThreads, const ISchedulerPtr& scheduler = {});
    static std::unique_ptr<ThreadPoolExecutorController>
    create(const ThreadPoolConfig& threadPoolConfig, const ISchedulerPtr& scheduler = {});

    ~ThreadPoolExecutorController() override;

    ThreadPoolExecutorController(ThreadPoolExecutorController const&) = delete;
    ThreadPoolExecutorController& operator=(ThreadPoolExecutorController const&) = delete;

    void finish() override;
    IExecutorPtr get_executor() const override;
    std::shared_ptr<IScheduler> get_scheduler() const override;

  private:
    ThreadPoolExecutorController(
        const ThreadPoolConfig& threadPoolConfig, const ISchedulerPtr& scheduler);

  private:
    std::shared_ptr<IThreadPoolExecutor> m_threadPoolExecutor;
    std::shared_ptr<IExecutor> m_executor;
    std::shared_ptr<SchedulerThread> m_schedulerThread;
    std::vector<std::thread> m_workerThreads;
    std::mutex m_stopMutex;
};

} // namespace asyncly
