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

#include <iterator>
#include <vector>

#include "asyncly/future/Future.h"
#include "asyncly/future/detail/WhenAll.h"

namespace asyncly {

template <typename T> class Future;

///
/// when_all can be used to combine multiple futures. It returns a
/// `Future` that will be resolved when all `Futures` that are
/// supplied to it are resolved. If any of those `Futures` are
/// rejected, the `Future` returned by `when_all` will also be
/// rejected with the first error that occured (in case of multiple
/// errors, any errors occuring after the first are discarded).
/// when_all accepts only rvalues as input arguments, i.e. the
/// input futures must be temporaries or passed using std::move().
///
/// Example usage:
/// \snippet WhenAllTest.cpp WhenAll Combination
///
/// \tparam Args deduced and must never be specified manually
/// \param args Futures to be combined
///
/// \return a `Future` containing a
/// std::tuple of the same type as the supplied promises, with void
/// promises filtered out. For example `when_all(Future<void>,
/// Future<int>, Future<void>, Future<bool>) ->
/// Future<std::tuple<int,bool>>`
///
template <typename... Args>
Future<detail::when_all_return_types<Args...>> when_all(Future<Args>&&... args)
{
    return { detail::when_all_impl(detail::get_future_impl(args)...) };
}

/// This overload of when all takes a range of Futures and returns a Future of a vector that will
/// contain all elements once all Futures are resolved or an error once the first Future is
/// rejected.
template <typename I> // I models InputIterator
Future<typename detail::when_all_iterator_result<
    typename std::iterator_traits<I>::value_type::value_type>::result_type>
when_all(I begin, I end)
{
    return { detail::when_all_iterator_impl(
        begin,
        end,
        detail::when_all_iterator_tag<
            typename std::iterator_traits<I>::value_type::value_type>{}) };
}

}
