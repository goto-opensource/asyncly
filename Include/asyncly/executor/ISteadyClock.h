/*
 * Copyright 2022 LogMeIn
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

#include <chrono>
#include <memory>

namespace asyncly {

class ISteadyClock {
  public:
    virtual ~ISteadyClock() = default;

    virtual std::chrono::steady_clock::time_point now() = 0;
};

using ISteadyClockPtr = std::shared_ptr<ISteadyClock>;
using ISteadyClockUPtr = std::unique_ptr<ISteadyClock>;

} // namespace asyncly
