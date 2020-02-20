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

#include "IRunnableScheduler.h"

#include <thread>

namespace asyncly {

class SchedulerThread {
  public:
    SchedulerThread(
        const ThreadInitFunction& threadInit,
        const std::shared_ptr<IRunnableScheduler>& runableScheduler);
    void finish();
    asyncly::ISchedulerPtr get_scheduler() const;

  private:
    const std::shared_ptr<IRunnableScheduler> m_runableScheduler;
    std::thread m_timerThread;
};

inline SchedulerThread::SchedulerThread(
    const ThreadInitFunction& threadInit,
    const std::shared_ptr<IRunnableScheduler>& runableScheduler)
    : m_runableScheduler(runableScheduler)
{
    m_timerThread = std::thread([this, threadInit] {
        if (threadInit) {
            threadInit();
        }
        m_runableScheduler->run();
    });
}

inline void SchedulerThread::finish()
{
    m_runableScheduler->stop();
    m_timerThread.join();
}

inline asyncly::ISchedulerPtr SchedulerThread::get_scheduler() const
{
    return m_runableScheduler;
}
}
