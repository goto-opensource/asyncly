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

#include <boost/asio.hpp>

#include "asyncly/task/Task.h"

namespace asyncly {
namespace detail {

class AsioTimerTask;

class AsioTimerHandler {
  public:
    AsioTimerHandler(asyncly::Task&& task, const std::shared_ptr<AsioTimerTask>& timerTask)
        : m_task{ std::move(task) }
        , m_timerTask{ timerTask }
    {
    }

    void operator()(const boost::system::error_code& error)
    {
        if (error) {
            return;
        }

        m_task();
    }

  private:
    asyncly::Task m_task;
    const std::shared_ptr<AsioTimerTask> m_timerTask;
};
}
}
