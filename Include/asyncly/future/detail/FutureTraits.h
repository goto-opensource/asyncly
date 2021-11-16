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

#include <boost/mp11.hpp>

#include <memory>
#include <tuple>
#include <type_traits>

namespace asyncly {
template <typename T> class Future;

namespace detail {
template <typename T> class FutureImpl;

template <typename T> struct future_traits {
    using is_future = boost::mp11::mp_false;
    using contains_tuple = boost::mp11::mp_false;
    using value_type = T;
};

template <typename T> struct future_traits<FutureImpl<T>> {
    using is_future = boost::mp11::mp_true;
    using contains_tuple = boost::mp11::mp_false;
    using value_type = T;
};

template <typename T> struct future_traits<Future<T>> {
    using is_future = boost::mp11::mp_true;
    using contains_tuple = boost::mp11::mp_false;
    using value_type = T;
};

template <typename T> struct future_traits<std::shared_ptr<FutureImpl<T>>> {
    using is_future = std::true_type;
    using contains_tuple = std::false_type;
    using value_type = T;
};

template <typename... Args> struct future_traits<FutureImpl<std::tuple<Args...>>> {
    using is_future = std::true_type;
    using contains_tuple = std::true_type;
    using value_type = std::tuple<Args...>;
};

template <typename... Args> struct future_traits<Future<std::tuple<Args...>>> {
    using is_future = std::true_type;
    using contains_tuple = std::true_type;
    using value_type = std::tuple<Args...>;
};

template <typename... Args> struct future_traits<std::shared_ptr<FutureImpl<std::tuple<Args...>>>> {
    using is_future = std::true_type;
    using contains_tuple = std::true_type;
    using value_type = std::tuple<Args...>;
};
} // namespace detail
} // namespace asyncly
