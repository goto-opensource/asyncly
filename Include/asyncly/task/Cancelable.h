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

namespace asyncly {

class Cancelable {
  public:
    virtual ~Cancelable() = default;
    ///
    /// Cancels a task.
    /// Cancellation cannot be guaranteed to be successful when called
    /// within another strand (or thread) than the task-to-be-cancelled is executed on.
    /// Reason: Task might already be in execution (or preparation of execution)
    ///
    /// \return  true if cancel succeeded, false otherwise (too late, already cancelled)
    virtual bool cancel() = 0;
};

} // namespace asyncly
