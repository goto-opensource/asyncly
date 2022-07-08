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

#include <memory>
#include <tuple>
#include <type_traits>

#include <boost/mp11.hpp>

#include <asyncly/future/detail/FutureTraits.h>

namespace asyncly::detail {

template <typename T> struct is_tuple {
    using type = boost::mp11::mp_false;
};

template <typename... Args> struct is_tuple<std::tuple<Args...>> {
    using type = boost::mp11::mp_true;
};

// tests for is_tuple
static_assert(is_tuple<std::tuple<int>>::type::value, "is_tuple<std::tuple<int>>");
static_assert(!is_tuple<bool>::type::value, "!is_tuple<bool>");

template <typename T, typename D = void> struct maybe_unpack_and_call;
template <typename T>
struct maybe_unpack_and_call<
    T,
    typename std::enable_if_t<!is_tuple<typename std::decay_t<T>>::type::value>> {
    template <typename F> auto operator()(F&& f, T value)
    {
        return f(std::move(value));
    }
};

template <typename F, typename T, typename... S>
auto maybe_unpack_and_call_(F&& f, T tuple, boost::mp11::mp_list<S...>)
{
    return f(std::get<S::value>(tuple)...);
}

template <typename T>
struct maybe_unpack_and_call<
    T,
    typename std::enable_if_t<is_tuple<typename std::decay_t<T>>::type::value>> {
    template <typename F> auto operator()(F&& f, T tuple)
    {
        return maybe_unpack_and_call_(
            std::forward<F>(f), tuple, typename boost::mp11::mp_iota_c<std::tuple_size_v<T>>{});
    }
};

template <typename F, typename T> struct continuation_result_type {
    using type = typename std::invoke_result_t<maybe_unpack_and_call<T>, F, T>;
};

template <typename F> struct continuation_result_type<F, void> {
    using type = typename std::invoke_result_t<F&&>;
};

template <typename T> class FutureImpl;

const auto void_lambda = []() {};
const auto int_lambda = []() { return 5; };
const auto bool_to_int_lambda = [](bool b) {
    std::ignore = b;
    return 5;
};
const auto tuple_to_void_lambda = [](bool, int, bool) {};
const auto future_returning_lambda = []() { return std::shared_ptr<FutureImpl<void>>{}; };

// tests for continuation_result_type
static_assert(
    std::is_same_v<continuation_result_type<decltype(void_lambda), void>::type, void>,
    "contiuation_result_type<decltype([](){})> == void");
static_assert(
    std::is_same_v<continuation_result_type<decltype(int_lambda), void>::type, int>,
    "contiuation_result_type<decltype([](){return 5;})> == int");
static_assert(
    std::is_same_v<continuation_result_type<decltype(bool_to_int_lambda), bool>::type, int>,
    "contiuation_result_type<decltype([](bool){return 5;})> == int");
static_assert(
    std::is_same_v<
        continuation_result_type<decltype(tuple_to_void_lambda), std::tuple<bool, int, bool>>::type,
        void>,
    "contiuation_result_type<decltype([](bool, int, bool){})> == void");
static_assert(
    std::is_same_v<
        continuation_result_type<decltype(future_returning_lambda), void>::type,
        std::shared_ptr<FutureImpl<void>>>,
    "contiuation_result_type<decltype([](){return make_ready_future<void>();})> == "
    "std::shared_ptr<FutureImpl<void>>");

template <typename T, typename F>
using continuation_future_element_type = typename boost::mp11::mp_if<
    typename future_traits<typename continuation_result_type<F, T>::type>::is_future,
    typename future_traits<typename continuation_result_type<F, T>::type>::value_type,
    typename continuation_result_type<F, T>::type>;

// tests for continuation_future_element_type
static_assert(
    std::is_same_v<continuation_future_element_type<void, decltype(int_lambda)>, int>,
    "continuation_future_element_type<decltype([](){return 5;})> == int");
static_assert(
    std::is_same_v<continuation_future_element_type<void, decltype(future_returning_lambda)>, void>,
    "contiuation_future_element_type<decltype([](){ return make_ready_future<void>(); })> == void");

template <typename T>
using null_tuple_to_void =
    typename boost::mp11::mp_if<boost::mp11::mp_same<std::tuple<>, T>, void, T>;

template <typename... Args>
using when_all_return_types = null_tuple_to_void<
    boost::mp11::mp_apply<std::tuple, boost::mp11::mp_remove<boost::mp11::mp_list<Args...>, void>>>;

// tests for when_all_return_types
static_assert(
    std::is_same_v<when_all_return_types<void, void, void>, void>,
    "when_all_return_types<void, void, void> == void");
static_assert(
    std::is_same_v<when_all_return_types<int, void, int>, std::tuple<int, int>>,
    "when_all_return_types<int, void, int> == std::tuple<int, int>");
} // namespace asyncly::detail
