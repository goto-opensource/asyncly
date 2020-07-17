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
#include <exception>
#include <map>
#include <mutex>

#include <function2/function2.hpp>

#include "asyncly/executor/CurrentExecutor.h"
#include "asyncly/observable/Subscriber.h"
#include "asyncly/observable/Subscription.h"

#include "asyncly/observable/detail/SharedSubscriptionContext.h"
#include "asyncly/observable/detail/Subscription.h"

namespace asyncly {

template <typename T> class Subscriber;
template <> class Subscriber<void>;

namespace detail {

template <typename T> class ObservableImpl {
  public:
    ObservableImpl(const ObservableImpl<T>&) = delete;
    ObservableImpl<T> operator=(const ObservableImpl<T>&) = delete;
    ObservableImpl(ObservableImpl<T>&&) = delete;
    ObservableImpl<T> operator=(ObservableImpl<T>&&) = delete;

  public:
    ObservableImpl(
        fu2::unique_function<void(asyncly::Subscriber<T>)> onSubscribe,
        const asyncly::IExecutorPtr& providerExecutor);

    asyncly::Subscription subscribe(fu2::unique_function<void(T)> valueFunction);
    asyncly::Subscription subscribe(
        fu2::unique_function<void(T)> valueFunction,
        fu2::unique_function<void(std::exception_ptr e)> errorFunction);
    asyncly::Subscription subscribe(
        fu2::unique_function<void(T)> valueFunction,
        fu2::unique_function<void(std::exception_ptr e)> errorFunction,
        fu2::unique_function<void()> completionFunction);

  private:
    struct ProviderContext {
        ProviderContext(fu2::unique_function<void(asyncly::Subscriber<T>)> conSubscribe)
            : onSubscribe{ std::move(conSubscribe) }
        {
        }

        fu2::unique_function<void(asyncly::Subscriber<T>)> onSubscribe;
    };

    const std::shared_ptr<ProviderContext> providerContext;
    const asyncly::IExecutorPtr providerExecutor;
};

template <> class ObservableImpl<void> {
  public:
    ObservableImpl(
        fu2::unique_function<void(asyncly::Subscriber<void>)> onSubscribe,
        const asyncly::IExecutorPtr& providerExecutor);

    asyncly::Subscription subscribe(fu2::unique_function<void()> valueFunction);
    asyncly::Subscription subscribe(
        fu2::unique_function<void()> valueFunction,
        fu2::unique_function<void(std::exception_ptr e)> errorFunction);
    asyncly::Subscription subscribe(
        fu2::unique_function<void()> valueFunction,
        fu2::unique_function<void(std::exception_ptr e)> errorFunction,
        fu2::unique_function<void()> completionFunction);

  private:
    struct ProviderContext {
        ProviderContext(fu2::unique_function<void(asyncly::Subscriber<void>)> conSubscribe)
            : onSubscribe{ std::move(conSubscribe) }
        {
        }

        fu2::unique_function<void(asyncly::Subscriber<void>)> onSubscribe;
    };

    const std::shared_ptr<ProviderContext> providerContext;
    const asyncly::IExecutorPtr providerExecutor;
};

template <typename T>
ObservableImpl<T>::ObservableImpl(
    fu2::unique_function<void(asyncly::Subscriber<T>)> onSubscribe,
    const asyncly::IExecutorPtr& cproviderExecutor)
    : providerContext{ std::make_shared<ProviderContext>(std::move(onSubscribe)) }
    , providerExecutor{ cproviderExecutor }
{
}

template <typename T>
asyncly::Subscription ObservableImpl<T>::subscribe(fu2::unique_function<void(T)> valueFunction)
{
    return subscribe(std::move(valueFunction), nullptr, nullptr);
}

template <typename T>
asyncly::Subscription ObservableImpl<T>::subscribe(
    fu2::unique_function<void(T)> valueFunction,
    fu2::unique_function<void(std::exception_ptr e)> errorFunction)
{
    return subscribe(std::move(valueFunction), std::move(errorFunction), nullptr);
}

template <typename T>
asyncly::Subscription ObservableImpl<T>::subscribe(
    fu2::unique_function<void(T)> valueFunction,
    fu2::unique_function<void(std::exception_ptr e)> errorFunction,
    fu2::unique_function<void()> completionFunction)
{
    auto subscriberExecutor = asyncly::this_thread::get_current_executor();
    auto subscriberContext = std::make_shared<detail::SharedSubscriptionContext<T>>(
        std::move(valueFunction), std::move(errorFunction), std::move(completionFunction));

    providerExecutor->post(
        [cproviderContext = this->providerContext, subscriberContext, subscriberExecutor]() {
            cproviderContext->onSubscribe(Subscriber<T>{ subscriberContext, subscriberExecutor });
        });
    return asyncly::Subscription{ std::make_shared<detail::Subscription<T>>(subscriberContext) };
}

inline ObservableImpl<void>::ObservableImpl(
    fu2::unique_function<void(asyncly::Subscriber<void>)> onSubscribe,
    const asyncly::IExecutorPtr& cproviderExecutor)
    : providerContext{ std::make_shared<ProviderContext>(std::move(onSubscribe)) }
    , providerExecutor{ cproviderExecutor }
{
}

inline asyncly::Subscription
ObservableImpl<void>::subscribe(fu2::unique_function<void()> valueFunction)
{
    return subscribe(std::move(valueFunction), nullptr, nullptr);
}

inline asyncly::Subscription ObservableImpl<void>::subscribe(
    fu2::unique_function<void()> valueFunction,
    fu2::unique_function<void(std::exception_ptr e)> errorFunction)
{
    return subscribe(std::move(valueFunction), std::move(errorFunction), nullptr);
}

inline asyncly::Subscription ObservableImpl<void>::subscribe(
    fu2::unique_function<void()> valueFunction,
    fu2::unique_function<void(std::exception_ptr e)> errorFunction,
    fu2::unique_function<void()> completionFunction)
{
    auto subscriberExecutor = asyncly::this_thread::get_current_executor();
    auto subscriberContext = std::make_shared<detail::SharedSubscriptionContext<void>>(
        std::move(valueFunction), std::move(errorFunction), std::move(completionFunction));

    providerExecutor->post([cproviderContext = this->providerContext,
                            subscriberContext,
                            subscriberExecutor]() {
        cproviderContext->onSubscribe(Subscriber<void>{ subscriberContext, subscriberExecutor });
    });
    return asyncly::Subscription{ std::make_shared<detail::Subscription<void>>(subscriberContext) };
}
}
}
