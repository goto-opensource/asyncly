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

#include "AsioTimerHandler.h"

#include "asyncly/ExecutorTypes.h"
#include "asyncly/task/Cancelable.h"
#include "asyncly/task/Task.h"

#include <boost/asio.hpp>

namespace asyncly {
namespace detail {

class AsioTimerTask : public asyncly::Cancelable,
                      public std::enable_shared_from_this<AsioTimerTask> {
  public:
    AsioTimerTask(asyncly::clock_type::duration duration, boost::asio::io_context& io_context)
        : timer_{ io_context, duration }
    {
    }

    void schedule(Task&& task)
    {
        // we need to retain a shared_ptr in the handler because asio holds a non-owning reference
        // to the timer
        timer_.async_wait(AsioTimerHandler{ std::move(task), shared_from_this() });
    }

    void cancel() override
    {
        timer_.cancel();
    }

  private:
    boost::asio::steady_timer timer_;
};
}
}
