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

#include "IRunnableScheduler.h"
#include "detail/BaseScheduler.h"
#include "detail/Sleep.h"

#include "asyncly/executor/IExecutor.h"
#include "asyncly/task/Task.h"

namespace asyncly {

class DefaultScheduler : public IRunnableScheduler {
  public:
    DefaultScheduler(const clock_type::duration& timerGranularity = std::chrono::milliseconds(5));

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
    BaseScheduler m_baseScheduler;
    const clock_type::duration m_timerGranularity;
    bool m_shutDownActive;
    std::mutex m_mutex;
};

inline DefaultScheduler::DefaultScheduler(const clock_type::duration& timerGranularity)
    : m_baseScheduler([]() { return clock_type::now(); })
    , m_timerGranularity(timerGranularity)
    , m_shutDownActive(false)
{
}

inline void DefaultScheduler::stop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_shutDownActive = true;
}

inline asyncly::clock_type::time_point DefaultScheduler::now() const
{
    return m_baseScheduler.now();
}

inline std::shared_ptr<Cancelable> DefaultScheduler::execute_at(
    const IExecutorWPtr& executor, const clock_type::time_point& absTime, Task&& task)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_baseScheduler.execute_at(executor, absTime, std::move(task));
}

inline std::shared_ptr<Cancelable> DefaultScheduler::execute_after(
    const IExecutorWPtr& executor, const clock_type::duration& relTime, Task&& task)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_baseScheduler.execute_after(executor, relTime, std::move(task));
}

inline void DefaultScheduler::run()
{
    // NOTE: There is no concurrent entrance in this function.
    // The mutex is required to protect the timer queue
    // against data races with the public interface.
    while (true) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_shutDownActive) {
                return;
            }
            m_baseScheduler.prepareElapse();
        }
        m_baseScheduler.elapse();
        asyncly::this_thread::sleep_for(m_timerGranularity);
    }
}
}
