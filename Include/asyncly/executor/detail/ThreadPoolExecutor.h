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

#include <boost/thread/condition_variable.hpp>
#include <mutex>
#include <queue>
#include <thread>

#include "asyncly/ExecutorTypes.h"
#include "asyncly/executor/IExecutor.h"
#include "asyncly/scheduler/IScheduler.h"
#include "asyncly/task/detail/PeriodicTask.h"

namespace asyncly {

class ThreadPoolExecutor : public IExecutor,
                           public std::enable_shared_from_this<ThreadPoolExecutor> {
  public:
    static std::shared_ptr<ThreadPoolExecutor>
    create(const asyncly::ISchedulerPtr& scheduler, bool isSerializing);

    ThreadPoolExecutor(ThreadPoolExecutor const&) = delete;
    ThreadPoolExecutor& operator=(ThreadPoolExecutor const&) = delete;

    void run();
    void finish();

    // IExecutor
    clock_type::time_point now() const override;
    void post(Task&&) override;
    std::shared_ptr<Cancelable> post_at(const clock_type::time_point& absTime, Task&&) override;
    std::shared_ptr<Cancelable> post_after(const clock_type::duration& relTime, Task&&) override;
    std::shared_ptr<Cancelable>
    post_periodically(const clock_type::duration& period, CopyableTask) override;
    ISchedulerPtr get_scheduler() const override;
    bool is_serializing() const override;

  private:
    ThreadPoolExecutor(const asyncly::ISchedulerPtr& scheduler, bool isSerializing);

  private:
    boost::mutex m_mutex;
    boost::condition_variable m_condition;
    unsigned int m_activeThreads;
    std::queue<Task> m_taskQueue;

    bool m_isShutdownActive;
    bool m_isStopped;

    const ISchedulerPtr m_scheduler;

    const bool m_isSerializing;
};

}
