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

#include "asyncly/executor/IExecutor.h"
#include "asyncly/scheduler/IScheduler.h"
#include "asyncly/scheduler/detail/BaseScheduler.h"
#include "asyncly/task/Task.h"

namespace asyncly {
namespace test {

class FakeClockScheduler : public IScheduler {
  public:
    FakeClockScheduler();

    bool advanceClockToNextEvent(clock_type::time_point limit);
    clock_type::time_point getLastExpiredTime() const;
    void clear();

    // IScheduler
    clock_type::time_point now() const override;

    std::shared_ptr<Cancelable> execute_at(
        const IExecutorWPtr& executor, const clock_type::time_point& absTime, Task&&) override;

    std::shared_ptr<Cancelable> execute_after(
        const IExecutorWPtr& executor, const clock_type::duration& relTime, Task&&) override;

  private:
    BaseScheduler m_baseScheduler;
    clock_type::time_point m_mockedNow;
    mutable std::mutex m_scheduledMutex;
    mutable std::mutex m_elapsedMutex;
};

inline FakeClockScheduler::FakeClockScheduler()
    : m_baseScheduler([this]() { return m_mockedNow; })
{
}

inline bool FakeClockScheduler::advanceClockToNextEvent(clock_type::time_point limit)
{
    // NOTE: Always take m_elapsedMutex before m_scheduledMutex
    std::unique_lock<std::mutex> lockE(m_elapsedMutex);
    {
        std::unique_lock<std::mutex> lockS(m_scheduledMutex);
        auto next = m_baseScheduler.getNextExpiredTime(limit);
        m_mockedNow = next;
        m_baseScheduler.prepareElapse();
    }
    m_baseScheduler.elapse();
    return (m_mockedNow >= limit);
}

inline clock_type::time_point FakeClockScheduler::getLastExpiredTime() const
{
    return m_baseScheduler.getLastExpiredTime();
}

inline void FakeClockScheduler::clear()
{
    // NOTE: Always take m_elapsedMutex before m_scheduledMutex
    std::unique_lock<std::mutex> lockE(m_elapsedMutex);
    std::unique_lock<std::mutex> lock(m_scheduledMutex);
    m_baseScheduler.clear();
}

inline asyncly::clock_type::time_point FakeClockScheduler::now() const
{
    std::unique_lock<std::mutex> lock(m_scheduledMutex);
    return m_mockedNow;
}

inline std::shared_ptr<Cancelable> FakeClockScheduler::execute_at(
    const IExecutorWPtr& executor, const clock_type::time_point& absTime, Task&& task)
{
    std::unique_lock<std::mutex> lock(m_scheduledMutex);
    return m_baseScheduler.execute_at(executor, absTime, std::move(task));
}

inline std::shared_ptr<Cancelable> FakeClockScheduler::execute_after(
    const IExecutorWPtr& executor, const clock_type::duration& relTime, Task&& task)
{
    std::unique_lock<std::mutex> lock(m_scheduledMutex);
    return m_baseScheduler.execute_after(executor, relTime, std::move(task));
}
}
}
