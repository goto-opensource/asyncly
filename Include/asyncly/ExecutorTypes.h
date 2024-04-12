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

#include <chrono>
#include <functional>
#include <memory>
#include <vector>

namespace asyncly {

using clock_type = std::chrono::steady_clock;

using AutoCancelablePtr = std::shared_ptr<class AutoCancelable>;
using CancelablePtr = std::shared_ptr<class Cancelable>;

class ISteadyClock;
using ISteadyClockPtr = std::shared_ptr<ISteadyClock>;

class IExecutor;
using IExecutorPtr = std::shared_ptr<IExecutor>;
using IExecutorWPtr = std::weak_ptr<IExecutor>;

class IScheduler;
using ISchedulerPtr = std::shared_ptr<IScheduler>;

class IStrand;
using IStrandPtr = std::shared_ptr<IStrand>;

class IExecutorController;
using IExecutorControllerUPtr = std::unique_ptr<IExecutorController>;

class SchedulerThread;

using ThreadInitFunction = std::function<void()>;

struct ThreadPoolConfig {
    std::vector<ThreadInitFunction> executorInitFunctions;
    ThreadInitFunction schedulerInitFunction;
};

struct ThreadConfig {
    ThreadInitFunction executorInitFunction;
    ThreadInitFunction schedulerInitFunction;
};

} // namespace asyncly
