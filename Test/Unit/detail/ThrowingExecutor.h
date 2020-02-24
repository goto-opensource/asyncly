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

#include "asyncly/executor/IExecutor.h"
#include "asyncly/scheduler/IScheduler.h"

#include <memory>
#include <stdexcept>

namespace asyncly {
namespace detail {

class ThrowingExecutor : public IExecutor, public std::enable_shared_from_this<ThrowingExecutor> {
  private:
    ThrowingExecutor();

  public:
    static std::shared_ptr<ThrowingExecutor> create();

    clock_type::time_point now() const override;
    void post(Task&& closure) override;
    std::shared_ptr<Cancelable> post_at(const clock_type::time_point&, Task&&) override;
    std::shared_ptr<Cancelable> post_after(const clock_type::duration&, Task&&) override;
    std::shared_ptr<Cancelable>
    post_periodically(const clock_type::duration&, CopyableTask) override;
    ISchedulerPtr get_scheduler() const override;
    bool is_serializing() const override;

  private:
    const ISchedulerPtr _scheduler;
};
class ThrowingScheduler : public IScheduler {
    clock_type::time_point now() const
    {
        return clock_type::now();
    }
    std::shared_ptr<Cancelable>
    execute_at(const IExecutorWPtr&, const clock_type::time_point&, Task&&)
    {
        throw std::runtime_error("throwing executor always throws");
    }
    std::shared_ptr<Cancelable>
    execute_after(const IExecutorWPtr&, const clock_type::duration&, Task&&)
    {
        throw std::runtime_error("throwing executor always throws");
    }
};

inline ThrowingExecutor::ThrowingExecutor()
    : _scheduler(std::make_shared<ThrowingScheduler>())
{
}

inline std::shared_ptr<ThrowingExecutor> ThrowingExecutor::create()
{
    return std::shared_ptr<ThrowingExecutor>(new ThrowingExecutor());
}

inline clock_type::time_point ThrowingExecutor::now() const
{
    return _scheduler->now();
}

inline void ThrowingExecutor::post(Task&&)
{
    throw std::runtime_error("throwing executor always throws");
}

inline std::shared_ptr<Cancelable> ThrowingExecutor::post_at(const clock_type::time_point&, Task&&)
{
    throw std::runtime_error("throwing executor always throws");
}

inline std::shared_ptr<Cancelable> ThrowingExecutor::post_after(const clock_type::duration&, Task&&)
{
    throw std::runtime_error("throwing executor always throws");
}

inline std::shared_ptr<Cancelable>
ThrowingExecutor::post_periodically(const clock_type::duration&, CopyableTask)
{
    throw std::runtime_error("throwing executor always throws");
}

inline ISchedulerPtr ThrowingExecutor::get_scheduler() const
{
    return _scheduler;
}

inline bool ThrowingExecutor::is_serializing() const
{
    return false;
}
}
}
