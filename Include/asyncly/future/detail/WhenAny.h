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

#include <atomic>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/utility.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// warning C4459: declaration of 'to' hides global declaration
#pragma warning(disable : 4459)
#endif

#include <boost/hana.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <boost/optional.hpp>
#include <boost/variant2/variant.hpp>

#include "asyncly/detail/TypeUtils.h"
#include "asyncly/future/detail/Future.h"

namespace asyncly::detail {

template <typename T> class PromiseImpl;
template <typename T> class FutureImpl;

template <typename... Args>
using when_any_unique_future_types = typename boost::mp11::mp_unique<
    boost::mp11::mp_list<typename future_traits<Args>::value_type...>>;

template <typename... Args>
using when_any_return_types = typename boost::mp11::mp_if<
    typename boost::mp11::mp_same<
        typename boost::mp11::mp_size<when_any_unique_future_types<Args...>>,
        boost::mp11::mp_size_t<1>>,
    typename boost::mp11::mp_at_c<when_any_unique_future_types<Args...>, 0>,
    typename boost::mp11::mp_apply<
        boost::variant2::variant,
        boost::mp11::mp_replace<when_any_unique_future_types<Args...>, void, boost::none_t>>>;

static_assert(
    std::is_same<when_any_return_types<Future<void>>, void>::value,
    "when_any_return_types<void> == void");
static_assert(
    std::is_same<when_any_return_types<Future<void>, Future<void>>, void>::value,
    "when_any_return_types<void, void> == void");
static_assert(
    std::is_same<when_any_return_types<Future<int>, Future<int>>, int>::value,
    "when_any_return_types<int, int> == int");
static_assert(
    std::is_same<
        when_any_return_types<Future<int>, Future<bool>>,
        boost::variant2::variant<int, bool>>::value,
    "when_any_return_types<int, bool> == boost::variant<int, bool>");
static_assert(
    std::is_same<
        when_any_return_types<Future<void>, Future<bool>>,
        boost::variant2::variant<boost::none_t, bool>>::value,
    "when_any_return_types<void, bool> == boost::variant<boost::none_t, bool>");

namespace {
template <typename... Args> struct when_any_resolver {
    when_any_resolver(
        const std::shared_ptr<PromiseImpl<when_any_return_types<Args...>>>& promise,
        const std::shared_ptr<std::atomic<bool>>& is_set)
        : promise_{ promise }
        , is_set_{ is_set }
    {
    }

    const std::shared_ptr<PromiseImpl<when_any_return_types<Args...>>> promise_;
    const std::shared_ptr<std::atomic<bool>> is_set_;

    template <typename T> void operator()(T&& value)
    {
        if (atomic_exchange(is_set_.get(), true) == false) {
            promise_->set_value(value);
        }
    }

    void operator()()
    {
        if (atomic_exchange(is_set_.get(), true) == false) {
            promise_->set_value(boost::none);
        }
    }
};
} // namespace

template <typename... Args>
std::shared_ptr<FutureImpl<when_any_return_types<Args...>>> when_any_impl(Args... args)
{
    auto lazy_future = make_lazy_future_impl<when_any_return_types<Args...>>();
    auto future = std::get<0>(lazy_future);
    auto promise = std::get<1>(lazy_future);
    auto is_set = std::make_shared<std::atomic<bool>>(false);

    auto resolver = std::make_shared<when_any_resolver<Args...>>(promise, is_set);

    boost::hana::for_each(boost::hana::make_tuple(args...), [promise, resolver, is_set](auto arg) {
        arg.then([resolver](auto... args) { return (*resolver)(args...); })
            .catch_error([promise, is_set](auto error) mutable {
                if (std::atomic_exchange(is_set.get(), true) == false) {
                    promise->set_exception(error);
                }
            });
    });

    return future;
}
} // namespace asyncly::detail
