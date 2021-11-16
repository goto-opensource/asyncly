/*
 * Copyright 2020 LogMeIn
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

#include "asyncly/executor/IStrand.h"

#include <memory>
namespace asyncly::test {

class IFakeExecutor : public IStrand {
  public:
    virtual ~IFakeExecutor() = default;
    /**
     * @param maxTasksToExecute default is 0 which means unlimited, > 0 means: execute only a
     * certain number of tasks in this call
     */
    virtual size_t runTasks(size_t maxTasksToExecute = 0) = 0;
    virtual void clear() = 0;
    virtual size_t queuedTasks() const = 0;
    virtual void advanceClock(clock_type::duration advance) = 0;
    virtual void advanceClock(clock_type::time_point timePoint) = 0;
};
using IFakeExecutorPtr = std::shared_ptr<IFakeExecutor>;

} // namespace asyncly::test