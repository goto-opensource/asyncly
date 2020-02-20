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

#include <boost/asio/io_context.hpp>

#include <mutex>

#include "IRunnableScheduler.h"
#include "PriorityQueue.h"
#include "detail/AsioTimerTask.h"

#include "asyncly/executor/IExecutor.h"
#include "asyncly/task/Task.h"

namespace asyncly {

class AsioScheduler : public IRunnableScheduler {
  public:
    AsioScheduler();

    // IScheduler
    clock_type::time_point now() const override;

    std::shared_ptr<Cancelable> execute_at(
        const IExecutorWPtr& executor, const clock_type::time_point& absTime, Task&&) override;

    std::shared_ptr<Cancelable> execute_after(
        const IExecutorWPtr& executor, const clock_type::duration& relTime, Task&&) override;

    // IRunnableScheduler
    void run() override;
    void stop() override;

  private:
    boost::asio::io_context m_ioContext;
    std::unique_ptr<boost::asio::io_context::work> m_work;
};

inline AsioScheduler::AsioScheduler()
    : m_work{ std::make_unique<boost::asio::io_context::work>(m_ioContext) }
{
}

inline asyncly::clock_type::time_point AsioScheduler::now() const
{
    return clock_type::now();
}

inline void AsioScheduler::stop()
{
    m_ioContext.stop();
    m_work.reset();
}

inline std::shared_ptr<Cancelable> AsioScheduler::execute_at(
    const IExecutorWPtr& executor, const clock_type::time_point& absTime, Task&& task)
{
    return execute_after(executor, absTime - clock_type::now(), std::move(task));
}

inline std::shared_ptr<Cancelable> AsioScheduler::execute_after(
    const IExecutorWPtr& executor, const clock_type::duration& relTime, Task&& task)
{
    auto timerTask = std::make_shared<detail::AsioTimerTask>(relTime, m_ioContext);
    timerTask->schedule([executor, task{ std::move(task) }]() mutable {
        if (auto p = executor.lock()) {
            p->post(std::move(task));
        }
    });
    return timerTask;
}

inline void AsioScheduler::run()
{
    m_ioContext.run();
}
}
