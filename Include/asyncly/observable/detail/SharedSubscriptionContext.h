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

#include <exception>
#include <mutex>
#include <utility>

#include <function2/function2.hpp>

namespace asyncly::detail {

enum class SubscriptionState {
    /// Active means there is a subscriber and values can be pushed
    Active,

    /// Unsubscribed means the subscription has been cancelled
    Unsubscribed,

    /// Completed means either pushError or complete has been called,
    /// which means no further value nor error nor completion callbacks
    /// must be called, according to the contract
    Completed,
};

/// SharedSubscriptionContext is shared between threads of producer and consumer of values, that's
/// why the mutex is necessary
template <typename T> struct SharedSubscriptionContext {
    SharedSubscriptionContext(
        fu2::unique_function<void(T)> cvalueFunction,
        fu2::unique_function<void(std::exception_ptr e)> cerrorFunction,
        fu2::unique_function<void()> ccompletionFunction)
        : state{ SubscriptionState::Active }
        , valueFunction{ std::move(cvalueFunction) }
        , errorFunction{ std::move(cerrorFunction) }
        , completionFunction{ std::move(ccompletionFunction) }
    {
    }

    void onSubscriptionCancelled()
    {
        std::lock_guard<std::mutex> lock{ mutex };
        state = SubscriptionState::Unsubscribed;
    }

    void onValue(T value)
    {
        std::lock_guard<std::mutex> lock{ mutex };
        if (state != SubscriptionState::Active || valueFunction == nullptr) {
            return;
        }
        valueFunction(std::move(value));
    }

    void onCompleted()
    {
        std::lock_guard<std::mutex> lock{ mutex };
        if (state != SubscriptionState::Active || completionFunction == nullptr) {
            return;
        }

        state = SubscriptionState::Completed;
        completionFunction();
    }

    void onError(std::exception_ptr e)
    {
        std::lock_guard<std::mutex> lock{ mutex };
        if (state != SubscriptionState::Active || errorFunction == nullptr) {
            return;
        }

        state = SubscriptionState::Completed;
        errorFunction(e);
    }

  private:
    SubscriptionState state;
    fu2::unique_function<void(T)> valueFunction;
    fu2::unique_function<void(std::exception_ptr e)> errorFunction;
    fu2::unique_function<void()> completionFunction;
    std::mutex mutex;
};

template <> struct SharedSubscriptionContext<void> {
    SharedSubscriptionContext(
        fu2::unique_function<void()> cvalueFunction,
        fu2::unique_function<void(std::exception_ptr e)> cerrorFunction,
        fu2::unique_function<void()> ccompletionFunction)
        : state{ SubscriptionState::Active }
        , valueFunction{ std::move(cvalueFunction) }
        , errorFunction{ std::move(cerrorFunction) }
        , completionFunction{ std::move(ccompletionFunction) }
    {
    }

    void onSubscriptionCancelled()
    {
        std::lock_guard<std::mutex> lock{ mutex };
        state = SubscriptionState::Unsubscribed;
    }

    void onValue()
    {
        std::lock_guard<std::mutex> lock{ mutex };
        if (state != SubscriptionState::Active || valueFunction == nullptr) {
            return;
        }
        valueFunction();
    }

    void onCompleted()
    {
        std::lock_guard<std::mutex> lock{ mutex };
        if (state != SubscriptionState::Active || completionFunction == nullptr) {
            return;
        }

        state = SubscriptionState::Completed;
        completionFunction();
    }

    void onError(std::exception_ptr e)
    {
        std::lock_guard<std::mutex> lock{ mutex };
        if (state != SubscriptionState::Active || errorFunction == nullptr) {
            return;
        }

        state = SubscriptionState::Completed;
        errorFunction(e);
    }

  private:
    SubscriptionState state;
    fu2::unique_function<void()> valueFunction;
    fu2::unique_function<void(std::exception_ptr e)> errorFunction;
    fu2::unique_function<void()> completionFunction;
    std::mutex mutex;
};
} // namespace asyncly::detail
