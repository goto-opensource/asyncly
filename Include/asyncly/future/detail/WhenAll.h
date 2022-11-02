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

#include <algorithm>
#include <exception>
#include <iterator>
#include <memory>
#include <optional>
#include <vector>

#include <boost/mp11.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// warning C4459: declaration of 'to' hides global declaration
#pragma warning(disable : 4459)
#endif

#include <boost/hana.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "asyncly/detail/TypeUtils.h"
#include "asyncly/future/detail/Future.h"

namespace asyncly::detail {

template <typename T> class FutureImpl;
template <typename T> class PromiseImpl;

namespace {

template <typename... Args>
using swap_void_for_bool = boost::mp11::mp_apply<
    std::tuple,
    boost::mp11::mp_transform<
        std::optional,
        boost::mp11::mp_replace<boost::mp11::mp_list<Args...>, void, bool>>>;

template <typename... Args> struct when_all_result_container {
    bool are_all_results_ready()
    {
        // todo: replace by `return hana::all(results);` once bug is fixed in msvc
        return boost::hana::fold(
            boost::hana::range_c<std::size_t, 0, sizeof...(Args)>,
            true,
            [this](bool state, auto index) -> bool {
                return state && std::get<index.value>(results);
            });
    };

    /** only call this when are_all_results_ready() == true, otherwise it will crash at runtime,
     * this was necessary to avoid allocations **/
    when_all_return_types<Args...> get_ready_results_unsafe()
    {
        auto types = boost::hana::to_tuple(boost::hana::tuple_t<Args...>);
        auto valueIndices = boost::hana::filter(
            boost::hana::to_tuple(boost::hana::make_range(
                boost::hana::size_c<0>, boost::hana::size_c<sizeof...(Args)>)),
            [types](auto index) {
                return boost::hana::at(types, index) != boost::hana::type_c<void>;
            });

        return boost::hana::unpack(valueIndices, [this](auto... indices) {
            return when_all_return_types<Args...>{ std::move(
                *std::get<indices.value>(results))... };
        });
    }

    template <typename T> void maybe_resolve_promise(T& promise)
    {
        if (alreadyContinued) {
            return;
        }

        if (!are_all_results_ready()) {
            return;
        }

        alreadyContinued = true;

        if constexpr (std::is_same_v<const std::shared_ptr<PromiseImpl<void>>, T>) {
            promise->set_value();
        } else {
            promise->set_value(get_ready_results_unsafe());
        }
    };

    template <typename T> void reject_all(std::exception_ptr error, T& promise)
    {
        if (alreadyContinued) {
            return;
        }

        alreadyContinued = true;

        promise->set_exception(error);
    }

    swap_void_for_bool<Args...> results;
    bool alreadyContinued = false;
};
} // namespace

template <typename... Args>
std::shared_ptr<FutureImpl<when_all_return_types<Args...>>>
when_all_impl(std::shared_ptr<FutureImpl<Args>>... args)
{
    auto result_container = std::make_shared<when_all_result_container<Args...>>();
    std::shared_ptr<FutureImpl<when_all_return_types<Args...>>> future;
    std::shared_ptr<PromiseImpl<when_all_return_types<Args...>>> promise;
    std::tie(future, promise) = make_lazy_future_impl<when_all_return_types<Args...>>();

    auto futures = boost::hana::make_tuple(args...);
    auto types = boost::hana::to_tuple(boost::hana::tuple_t<Args...>);
    auto indexRange = boost::hana::to_tuple(
        boost::hana::make_range(boost::hana::size_c<0>, boost::hana::size_c<sizeof...(Args)>));

    auto valueIndices = boost::hana::filter(indexRange, [types](auto index) {
        return boost::hana::at(types, index) != boost::hana::type_c<void>;
    });

    auto voidIndices = boost::hana::filter(indexRange, [types](auto index) {
        return boost::hana::at(types, index) == boost::hana::type_c<void>;
    });

    boost::hana::for_each(valueIndices, [&result_container, &promise, &futures](auto index) {
        auto future = boost::hana::at(futures, index);
        using type = typename std::add_const_t<
            typename std::decay_t<decltype(future)>::element_type::value_type>;
        future
            ->then([result_container, promise, index](type v) {
                std::get<index.value>(result_container->results) = v;
                result_container->maybe_resolve_promise(promise);
            })
            ->catch_error([result_container, promise](std::exception_ptr error) {
                result_container->reject_all(error, promise);
            });
    });

    boost::hana::for_each(voidIndices, [&result_container, &promise, &futures](auto index) {
        auto future = boost::hana::at(futures, index);
        future
            ->then([result_container, promise, index]() {
                std::get<index.value>(result_container->results) = true;
                result_container->maybe_resolve_promise(promise);
            })
            ->catch_error([result_container, promise](std::exception_ptr error) {
                result_container->reject_all(error, promise);
            });
    });

    return future;
}

template <typename T> struct when_all_iterator_result {
    using result_type = typename std::vector<T>;
};

template <> struct when_all_iterator_result<void> {
    using result_type = void;
};

template <typename> struct when_all_iterator_tag { };

template <typename I> // I models InputIterator<Future<void>>
std::shared_ptr<FutureImpl<void>>
when_all_iterator_impl(I begin, I end, when_all_iterator_tag<void>)
{
    auto size = std::distance(begin, end);

    auto shouldWeResolveImmediately = size == 0;
    if (shouldWeResolveImmediately) {
        return make_ready_future_impl();
    }

    struct State {
        State(std::size_t size, std::shared_ptr<PromiseImpl<void>> p)
            : values(size, false)
            , promise(std::move(p))
        {
        }

        void maybe_resolve_or_reject()
        {
            if (done) {
                return;
            }

            auto areAllValuesComplete = std::all_of(
                values.begin(), values.end(), [](const auto& value) { return value; });

            if (areAllValuesComplete) {
                promise->set_value();
                done = true;
                return;
            }

            if (error != nullptr) {
                promise->set_exception(error);
                done = true;
                return;
            }
        }

        std::exception_ptr error = nullptr;
        std::vector<bool> values;
        std::shared_ptr<PromiseImpl<void>> promise;
        bool done = false;
        std::mutex mutex;
    };

    std::shared_ptr<FutureImpl<void>> resultFuture;
    std::shared_ptr<PromiseImpl<void>> promise;
    std::tie(resultFuture, promise) = make_lazy_future_impl<void>();

    auto state = std::make_shared<State>(std::distance(begin, end), std::move(promise));

    auto index = std::size_t{ 0 };
    std::for_each(begin, end, [&state, &index](auto future) {
        std::move(future)
            .then([state, index]() {
                std::lock_guard<std::mutex> lock{ state->mutex };
                state->values[index] = true;
                state->maybe_resolve_or_reject();
            })
            .catch_error([state](auto e) {
                std::lock_guard<std::mutex> lock{ state->mutex };
                state->error = e;
                state->maybe_resolve_or_reject();
            });
        index++;
    });

    return resultFuture;
}

template <typename I, typename T> // I models InputIterator<Future<T>>
std::shared_ptr<
    FutureImpl<typename std::vector<typename std::iterator_traits<I>::value_type::value_type>>>
when_all_iterator_impl(I begin, I end, when_all_iterator_tag<T>)
{
    static_assert(!std::is_void_v<T>, "T must not be void");
    using FutureT = typename std::iterator_traits<I>::value_type;
    using ValueT = typename FutureT::value_type;

    auto size = std::distance(begin, end);

    auto shouldWeResolveImmediatelyWithAnEmptyVector = size == 0;
    if (shouldWeResolveImmediatelyWithAnEmptyVector) {
        return make_ready_future_impl<std::vector<ValueT>>({});
    }

    struct State {
        State(std::size_t size, std::shared_ptr<PromiseImpl<std::vector<ValueT>>> p)
            : values(size)
            , promise(std::move(p))
        {
        }

        void maybe_resolve_or_reject()
        {
            if (done) {
                return;
            }

            auto areAllValuesComplete = std::all_of(
                values.begin(), values.end(), [](const auto& value) { return value.has_value(); });

            if (areAllValuesComplete) {
                auto resultValues = std::vector<ValueT>{};
                std::transform(
                    std::make_move_iterator(values.begin()),
                    std::make_move_iterator(values.end()),
                    std::back_inserter(resultValues),
                    [](std::optional<ValueT>&& value) -> ValueT { return *std::move(value); });
                promise->set_value(std::move(resultValues));
                done = true;
                return;
            }

            if (error != nullptr) {
                promise->set_exception(error);
                done = true;
                return;
            }
        }

        std::exception_ptr error = nullptr;
        std::vector<std::optional<ValueT>> values;
        std::shared_ptr<PromiseImpl<std::vector<ValueT>>> promise;
        bool done = false;
        std::mutex mutex;
    };

    std::shared_ptr<FutureImpl<std::vector<ValueT>>> resultFuture;
    std::shared_ptr<PromiseImpl<std::vector<ValueT>>> promise;
    std::tie(resultFuture, promise) = make_lazy_future_impl<std::vector<ValueT>>();

    auto state = std::make_shared<State>(size, std::move(promise));

    auto index = std::size_t{ 0 };
    std::for_each(begin, end, [&state, &index](auto future) {
        std::move(future)
            .then([state, index](auto value) {
                std::lock_guard<std::mutex> lock{ state->mutex };
                state->values[index] = std::move(value);
                state->maybe_resolve_or_reject();
            })
            .catch_error([state](auto e) {
                std::lock_guard<std::mutex> lock{ state->mutex };
                state->error = e;
                state->maybe_resolve_or_reject();
            });
        index++;
    });

    return resultFuture;
}

} // namespace asyncly::detail
