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

#include "asyncly/executor/IStrand.h"

#include "asyncly/scheduler/IScheduler.h"
#include "asyncly/task/Task.h"
#include "asyncly/task/detail/PeriodicTask.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <functional>

namespace asyncly {

class AsioExecutor final : public IStrand, public std::enable_shared_from_this<AsioExecutor> {
  public:
    explicit AsioExecutor(const ISchedulerPtr& scheduler);
    ~AsioExecutor() override;
    AsioExecutor(AsioExecutor const&) = delete;
    AsioExecutor& operator=(AsioExecutor const&) = delete;

    void run();
    void finish();
    boost::asio::io_context& get_io_context();

    // IExecutor
    clock_type::time_point now() const override;
    void post(Task&&) override;
    std::shared_ptr<Cancelable> post_at(const clock_type::time_point& absTime, Task&&) override;
    std::shared_ptr<Cancelable> post_after(const clock_type::duration& relTime, Task&&) override;
    std::shared_ptr<Cancelable>
    post_periodically(const clock_type::duration& relTime, RepeatableTask&&) override;
    ISchedulerPtr get_scheduler() const override;

  private:
    boost::asio::io_context ioContext_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;
    const ISchedulerPtr m_scheduler;
};
}
