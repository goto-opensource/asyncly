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

#include <future>

namespace asyncly::test {

inline void postAndWait(
    const asyncly::IStrandPtr& strand,
    const asyncly::clock_type::duration& timeout,
    const std::function<void()>& task)
{
    std::promise<void> synchronized;
    auto synchronize = [&synchronized, task]() {
        task();
        synchronized.set_value();
    };

    strand->post(synchronize);

    if (synchronized.get_future().wait_for(timeout) == std::future_status::timeout) {
        throw std::runtime_error("Timeout waiting for Task execution");
    }
}

template <typename T>
T postWaitGet(
    const asyncly::IStrandPtr& strand,
    const asyncly::clock_type::duration& timeout,
    const std::function<T()>& task)
{
    std::promise<T> synchronized;
    auto synchronize = [&synchronized, task]() { synchronized.set_value(task()); };

    strand->post(synchronize);

    auto future = synchronized.get_future();
    if (future.wait_for(timeout) == std::future_status::timeout) {
        throw std::runtime_error("Timeout waiting for Task execution");
    }
    return future.get();
}

} // namespace asyncly::test
