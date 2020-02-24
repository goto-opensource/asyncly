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

#include "gmock/gmock.h"

namespace asyncly {

class MockIExecutor : public IExecutor {
  public:
    // gmock mocks doen't work with rvalue references, so forward to lvalue reference overloads
    void post(Task&& task) override
    {
        post(task);
    }

    std::shared_ptr<Cancelable> post_at(const clock_type::time_point& absTime, Task&& task) override
    {
        return post_at(absTime, task);
    }

    std::shared_ptr<Cancelable>
    post_after(const clock_type::duration& relTime, Task&& task) override
    {
        return post_after(relTime, task);
    }

    // asyncly::IExecutor
    MOCK_CONST_METHOD0(now, clock_type::time_point());
    MOCK_METHOD1(post, void(const asyncly::Task& arg1));
    MOCK_METHOD2(
        post_at,
        std::shared_ptr<Cancelable>(
            const clock_type::time_point& absTime, const asyncly::Task& arg1));
    MOCK_METHOD2(
        post_after,
        std::shared_ptr<Cancelable>(
            const clock_type::duration& relTime, const asyncly::Task& arg1));
    MOCK_METHOD2(
        post_periodically,
        std::shared_ptr<Cancelable>(const clock_type::duration& period, CopyableTask task));
    MOCK_CONST_METHOD0(get_scheduler, ISchedulerPtr());
    MOCK_CONST_METHOD0(is_serializing, bool());
};
}
