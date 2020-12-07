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

#include "asyncly/executor/InlineExecutor.h"

namespace asyncly {

class InlineScheduler : public IScheduler {
    clock_type::time_point now() const override
    {
        return clock_type::now();
    }
    std::shared_ptr<Cancelable>
    execute_at(const IExecutorWPtr&, const clock_type::time_point&, Task&&) override
    {
        assert(false);
        return std::shared_ptr<Cancelable>();
    }
    std::shared_ptr<Cancelable>
    execute_after(const IExecutorWPtr&, const clock_type::duration&, Task&&) override
    {
        assert(false);
        return std::shared_ptr<Cancelable>();
    }
};

InlineExecutor::InlineExecutor()
    : _scheduler(std::make_shared<InlineScheduler>())
{
}

std::shared_ptr<InlineExecutor> InlineExecutor::create()
{
    return std::shared_ptr<InlineExecutor>(new InlineExecutor());
}

clock_type::time_point InlineExecutor::now() const
{
    return _scheduler->now();
}

void InlineExecutor::post(Task&& closure)
{
    closure.maybe_set_executor(shared_from_this());
    closure();
}

std::shared_ptr<Cancelable> InlineExecutor::post_at(const clock_type::time_point&, Task&&)
{
    assert(false);
    return std::shared_ptr<Cancelable>();
}

std::shared_ptr<Cancelable> InlineExecutor::post_after(const clock_type::duration&, Task&&)
{
    assert(false);
    return std::shared_ptr<Cancelable>();
}

std::shared_ptr<Cancelable>
InlineExecutor::post_periodically(const clock_type::duration&, CopyableTask)
{
    assert(false);
    return std::shared_ptr<Cancelable>();
}

ISchedulerPtr InlineExecutor::get_scheduler() const
{
    return _scheduler;
}

}