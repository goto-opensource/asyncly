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

#include "IExecutorController.h"
#include "asyncly/ExecutorTypes.h"
#include "asyncly/scheduler/DefaultScheduler.h"
#include "asyncly/scheduler/SchedulerThread.h"
#include "detail/AsioExecutor.h"

#include <boost/asio/io_context.hpp>

#include <functional>
#include <thread>

namespace asyncly {
class AsioExecutorController : public IExecutorController {
  public:
    static std::unique_ptr<AsioExecutorController>
    create(const ThreadConfig& threadConfig = {}, const ISchedulerPtr& scheduler = {});

    ~AsioExecutorController();
    AsioExecutorController(AsioExecutorController const&) = delete;
    AsioExecutorController& operator=(AsioExecutorController const&) = delete;

    // IExecutorControl
    void finish() override;
    std::shared_ptr<IExecutor> get_executor() const override;
    std::shared_ptr<IScheduler> get_scheduler() const override;

    boost::asio::io_context& get_io_context();

  private:
    AsioExecutorController(
        const ThreadConfig& threadConfig, const ISchedulerPtr& optionalScheduler);
    std::shared_ptr<AsioExecutor> m_executor;
    std::thread m_workerThread;
    std::shared_ptr<SchedulerThread> m_schedulerThread;
    std::mutex m_stopMutex;
};

}
