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

#include "asyncly/future/detail/WhenAny.h"

namespace asyncly {

template <typename T> class Future;

///
/// when_any can be used to combine multiple `Futures` into another
/// `Future` that will be resolved when the first of the supplied
/// `Future` is resolved or rejected when any `Future` is rejected
/// (whichever happens first).
///
/// Example usage:
/// \snippet WhenAnyTest.cpp WhenAny Combination
///
/// \tparam Args deduced and must never be specified manually
/// \param args Futures to be combined
///
/// \return a `Future` containing a `boost::variant` of possible
/// values. The `variant` types are ordered by a types first
/// occurrance in the passed arguments. When two `Futures` of the same
/// type are contained in `args`, it's no longer possible to
/// distinguish which `Future` has been resolved. `void` is replaced
/// by `boost::none_t`, because `variants` can not hold void. Example:
/// `when_any(Future<int>, Future<void>, Future<int>, Future<bool>) ->
/// Future<boost::variant<int, boost::none_t, bool>`
///
template <typename... Args> Future<detail::when_any_return_types<Args...>> when_any(Args... args)
{
    return { detail::when_any_impl(args...) };
}
}
