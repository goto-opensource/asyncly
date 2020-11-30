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

#include <thread>

#include "boost/asio/io_context.hpp"

#include "asyncly/executor/AsioExecutorController.h"
#include "asyncly/executor/ExternalEventExecutorController.h"
#include "asyncly/executor/IExecutor.h"
#include "asyncly/executor/ThreadPoolExecutorController.h"
#include "asyncly/scheduler/AsioScheduler.h"
#include "asyncly/test/SchedulerProvider.h"

namespace asyncly {
namespace test {

using SchedulerProviderDefault = SchedulerProviderExternal<DefaultScheduler>;
using SchedulerProviderAsio = SchedulerProviderExternal<AsioScheduler>;

template <class SchedulerProvider = SchedulerProviderNone> class AsioExecutorFactory {
  public:
    AsioExecutorFactory()
    {
        executorController_
            = asyncly::AsioExecutorController::create({}, schedulerProvider_.get_scheduler());
    }

    std::shared_ptr<IExecutor> create()
    {
        return executorController_->get_executor();
    }

  private:
    std::unique_ptr<IExecutorController> executorController_;
    SchedulerProvider schedulerProvider_;
};

template <size_t threads = 1, class SchedulerProvider = SchedulerProviderNone>
class DefaultExecutorFactory {
  public:
    DefaultExecutorFactory()
    {
        executorController_
            = ThreadPoolExecutorController::create(threads, schedulerProvider_.get_scheduler());
    }

    IExecutorPtr create()
    {
        return executorController_->get_executor();
    }

  private:
    std::unique_ptr<IExecutorController> executorController_;
    SchedulerProvider schedulerProvider_;
};

template <class SchedulerProvider = SchedulerProviderNone> class ExternalEventExecutorFactory {
  public:
    ExternalEventExecutorFactory()
    {
        executorControllerExternal_
            = ThreadPoolExecutorController::create(1, schedulerProvider_.get_scheduler());

        auto externalEventFunction = [this] {
            executorControllerExternal_->get_executor()->post(
                [this] { executorController_->runOnce(); });
        };

        executorController_ = asyncly::ExternalEventExecutorController::create(
            externalEventFunction, [] {}, schedulerProvider_.get_scheduler());
    }
    ~ExternalEventExecutorFactory()
    {
        executorControllerExternal_->finish();
    }

    std::shared_ptr<IExecutor> create()
    {
        return executorController_->get_executor();
    }

  private:
    std::unique_ptr<IExecutorController> executorControllerExternal_;
    std::unique_ptr<ExternalEventExecutorController> executorController_;
    SchedulerProvider schedulerProvider_;
};

}
}
