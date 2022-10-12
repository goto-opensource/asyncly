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

#include "asyncly/future/Future.h"

namespace asyncly {
template <typename T> auto when_then(Future<T>&& when, Promise<T>& then)
{
    when.then([then](T val) mutable { then.set_value(val); })
        .catch_error([then](std::exception_ptr e) mutable { then.set_exception(e); });
}
template <> inline auto when_then<void>(Future<void>&& when, Promise<void>& then)
{
    when.then([then]() mutable { then.set_value(); })
        .catch_error([then](std::exception_ptr e) mutable { then.set_exception(e); });
}
} // namespace asyncly
