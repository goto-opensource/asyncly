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
#include <memory>
#include <tuple>

#include "asyncly/executor/IExecutor.h"

#include "asyncly/observable/detail/SharedSubscriptionContext.h"

namespace asyncly {

namespace detail {
template <typename T> class ObservableImpl;

enum class SubscriberState {
    // Active means no complete() or pushError() has been called on this
    Active,
    // Completed means complete() or pushError() has been called on this, no
    // invokation of callbacks is allowed afterwards
    Completed,
};
}

template <typename T> class Subscriber;
template <typename T> class Observable;

/// Subscriber can be used to send values to Observables
/// Multiple values can be pushed, but after a call to complete or pushError, no more
/// calls to either pushValue, complete or pushError are allowed and will result in an
/// std::runtime_error being thrown.
template <typename T> class Subscriber {
  public:
    /// pushValue sends a value to the onValue handlers of subcribers of the associated Observable
    void pushValue(T value);

    /// complete signals no more values will be sent to subscribers of the associated Observable
    void complete();

    /// pushError signals an error to subscribers of the associated Observable. Neither more values
    /// nor more errors nor a complete() indication will follow an error.
    void pushError(std::exception_ptr e);

  public:
    friend class detail::ObservableImpl<T>;

  private:
    Subscriber(
        const std::shared_ptr<detail::SharedSubscriptionContext<T>>& ccontext,
        const asyncly::IExecutorPtr& cexecutor);

    std::shared_ptr<detail::SharedSubscriptionContext<T>> context;
    asyncly::IExecutorPtr executor;
    detail::SubscriberState state;
};

template <> class Subscriber<void> {
  public:
    /// pushValue sends a value to the onValue handlers of subcribers of the associated Observable
    void pushValue();

    /// complete signals no more values will be sent to subscribers of the associated Observable
    void complete();

    /// pushError signals an error to subscribers of the associated Observable. Neither more values
    /// nor more errors nor a complete() indication will follow an error.
    void pushError(std::exception_ptr e);

  public:
    friend class detail::ObservableImpl<void>;

  private:
    Subscriber(
        const std::shared_ptr<detail::SharedSubscriptionContext<void>>& ccontext,
        const asyncly::IExecutorPtr& cexecutor);

    std::shared_ptr<detail::SharedSubscriptionContext<void>> context;
    asyncly::IExecutorPtr executor;
    detail::SubscriberState state;
};

template <typename T>
Subscriber<T>::Subscriber(
    const std::shared_ptr<detail::SharedSubscriptionContext<T>>& ccontext,
    const asyncly::IExecutorPtr& cexecutor)
    : context{ ccontext }
    , executor{ cexecutor }
    , state{ detail::SubscriberState::Active }
{
}

template <typename T> void Subscriber<T>::pushValue(T value)
{
    if (state != detail::SubscriberState::Active) {
        throw std::runtime_error{
            "No completion, error or value must be emmitted after completion or error"
        };
    }

    executor->post([value, ccontext = this->context]() { ccontext->onValue(std::move(value)); });
}

template <typename T> void Subscriber<T>::complete()
{
    if (state != detail::SubscriberState::Active) {
        throw std::runtime_error{
            "No completion, error or value must be emmitted after completion or error"
        };
    }
    state = detail::SubscriberState::Completed;

    executor->post([ccontext = this->context]() { ccontext->onCompleted(); });
}

template <typename T> void Subscriber<T>::pushError(std::exception_ptr e)
{
    if (state != detail::SubscriberState::Active) {
        throw std::runtime_error{
            "No completion, error or value must be emmitted after completion or error"
        };
    }

    executor->post([ccontext = this->context, e]() { ccontext->onError(e); });
}

inline Subscriber<void>::Subscriber(
    const std::shared_ptr<detail::SharedSubscriptionContext<void>>& ccontext,
    const asyncly::IExecutorPtr& cexecutor)
    : context{ ccontext }
    , executor{ cexecutor }
    , state{ detail::SubscriberState::Active }
{
}

inline void Subscriber<void>::pushValue()
{
    if (state != detail::SubscriberState::Active) {
        throw std::runtime_error{
            "No completion, error or value must be emmitted after completion or error"
        };
    }

    executor->post([ccontext = this->context]() { ccontext->onValue(); });
}

inline void Subscriber<void>::complete()
{
    if (state != detail::SubscriberState::Active) {
        throw std::runtime_error{
            "No completion, error or value must be emmitted after completion or error"
        };
    }
    state = detail::SubscriberState::Completed;

    executor->post([ccontext = this->context]() { ccontext->onCompleted(); });
}

inline void Subscriber<void>::pushError(std::exception_ptr e)
{
    if (state != detail::SubscriberState::Active) {
        throw std::runtime_error{
            "No completion, error or value must be emmitted after completion or error"
        };
    }

    executor->post([ccontext = this->context, e]() { ccontext->onError(e); });
}
}
