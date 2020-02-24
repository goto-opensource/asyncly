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

#include <memory>
#include <string>

#include "asyncly/ExecutorTypes.h"
#include "asyncly/scheduler/SchedulerThread.h"

namespace asyncly {
namespace test {

class SchedulerProviderNone {
  public:
    ISchedulerPtr get_scheduler() const
    {
        return ISchedulerPtr();
    }
};

template <class SchedulerImpl> class SchedulerProviderExternal {
  public:
    SchedulerProviderExternal()
    {
        schedulerThread_ = std::make_shared<SchedulerThread>(
            ThreadInitFunction{}, std::make_shared<SchedulerImpl>());
    }
    virtual ~SchedulerProviderExternal()
    {
        if (schedulerThread_) {
            schedulerThread_->finish();
        }
    }

    ISchedulerPtr get_scheduler() const
    {
        return schedulerThread_->get_scheduler();
    }

  private:
    std::shared_ptr<SchedulerThread> schedulerThread_;
};
}
}
