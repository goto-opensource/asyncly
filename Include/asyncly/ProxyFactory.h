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

#include "asyncly/ExecutorTypes.h"
#include "asyncly/proxy/policy/RawOwnershipPolicy.h"
#include "asyncly/proxy/policy/StrongOwnershipPolicy.h"
#include "asyncly/proxy/policy/WeakOwnershipPolicy.h"
#include "asyncly/proxy/policy/WeakerOwnershipPolicy.h"
#include "asyncly/task/Task.h"

#include <memory>

namespace asyncly {

/**
 * Factory class for creating executor proxies
 */
template <class IUser> class ProxyFactory {
  public:
    static std::unique_ptr<IUser> createStrongProxy(
        std::shared_ptr<IUser> callee, IExecutorPtr executor, asyncly::Task&& onProxyDelete = []() {
        });

    static std::unique_ptr<IUser> createWeakProxy(
        std::weak_ptr<IUser> callee, IExecutorPtr executor, asyncly::Task&& onProxyDelete = []() {
        });

    static std::unique_ptr<IUser> createRawProxy(
        IUser* callee, IExecutorPtr executor, asyncly::Task&& onProxyDelete = []() {});

    static std::unique_ptr<IUser> createWeakerProxy(
        std::weak_ptr<IUser> callee, IExecutorPtr executor, asyncly::Task&& onProxyDelete = []() {
        });
};
}

#define ASYNCLY_DECLARE_PROXY_FACTORY_METHOD(Interface, Proxy)                                     \
    namespace asyncly {                                                                            \
    template <>                                                                                    \
    std::unique_ptr<Interface> asyncly::ProxyFactory<Interface>::createStrongProxy(                \
        std::shared_ptr<Interface> callee, IExecutorPtr executor, asyncly::Task&& onProxyDelete)   \
    {                                                                                              \
        auto proxy = std::unique_ptr<Interface>(                                                   \
            new Proxy<StrongOwnershipPolicy<std::shared_ptr<Interface>>>(                          \
                std::move(callee), std::move(executor), std::move(onProxyDelete)));                \
        return proxy;                                                                              \
    }                                                                                              \
    template <>                                                                                    \
    std::unique_ptr<Interface> asyncly::ProxyFactory<Interface>::createWeakProxy(                  \
        std::weak_ptr<Interface> callee,                                                           \
        asyncly::IExecutorPtr executor,                                                            \
        asyncly::Task&& onProxyDelete)                                                             \
    {                                                                                              \
        auto proxy                                                                                 \
            = std::unique_ptr<Interface>(new Proxy<WeakOwnershipPolicy<std::weak_ptr<Interface>>>( \
                std::move(callee), std::move(executor), std::move(onProxyDelete)));                \
        return proxy;                                                                              \
    }                                                                                              \
    template <>                                                                                    \
    std::unique_ptr<Interface> asyncly::ProxyFactory<Interface>::createRawProxy(                   \
        Interface* callee, asyncly::IExecutorPtr executor, asyncly::Task&& onProxyDelete)          \
    {                                                                                              \
        auto proxy = std::unique_ptr<Interface>(new Proxy<RawOwnershipPolicy<Interface*>>(         \
            std::move(callee), std::move(executor), std::move(onProxyDelete)));                    \
        return proxy;                                                                              \
    }                                                                                              \
    template <>                                                                                    \
    std::unique_ptr<Interface> asyncly::ProxyFactory<Interface>::createWeakerProxy(                \
        std::weak_ptr<Interface> callee,                                                           \
        asyncly::IExecutorPtr executor,                                                            \
        asyncly::Task&& onProxyDelete)                                                             \
    {                                                                                              \
        auto proxy = std::unique_ptr<Interface>(                                                   \
            new Proxy<WeakerOwnershipPolicy<std::weak_ptr<Interface>>>(                            \
                std::move(callee), std::move(executor), std::move(onProxyDelete)));                \
        return proxy;                                                                              \
    }                                                                                              \
    }
