/*
 * Copyright 2022 GoTo
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

#include "asyncly/Future.h"
#include "asyncly/future/Future.h"

#include <optional>
#include <stdexcept>
#include <tuple>

namespace asyncly {

template <typename ValueType> class LazyValue {
  public:
    LazyValue()
        : _future(_promise.get_future())
        , _hasValue(false)
    {
    }

    LazyValue(const LazyValue&) = delete;
    LazyValue(LazyValue&&) = delete;
    LazyValue& operator=(const LazyValue&) = delete;
    LazyValue& operator=(LazyValue&&) = delete;

    ~LazyValue()
    {
        if (!_hasValue) {
            _promise.set_exception(std::runtime_error("Could not be resolved. No value was set."));
        }
    }

    asyncly::Future<ValueType> get_future()
    {
        auto [futureToReturn, futureToStore] = asyncly::split(std::move(*_future));
        _future.emplace(futureToStore);
        return futureToReturn;
    }

    void set_value(const ValueType& value)
    {
        _hasValue = true;
        _promise.set_value(value);
    }

    void set_value(ValueType&& value)
    {
        _hasValue = true;
        _promise.set_value(std::move(value));
    }

    bool has_value() const
    {
        return _hasValue;
    }

  private:
    asyncly::Promise<ValueType> _promise;
    std::optional<asyncly::Future<ValueType>> _future;

    bool _hasValue;
};

} // namespace asyncly