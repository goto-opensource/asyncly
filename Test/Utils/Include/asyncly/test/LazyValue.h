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

namespace asyncly::test {

template <typename ValueType> class LazyValue {
  public:
    LazyValue()
        : _valueFuturePromise(asyncly::make_lazy_future<ValueType>())
    {
    }

    asyncly::Future<ValueType> get_future() const
    {
        return std::get<0>(_valueFuturePromise);
    }

    void set_value(const ValueType& value)
    {
        std::get<1>(_valueFuturePromise).set_value(value);
    }

    void set_value(ValueType&& value)
    {
        std::get<1>(_valueFuturePromise).set_value(std::move(value));
    }

  private:
    std::tuple<asyncly::Future<ValueType>, asyncly::Promise<ValueType>> _valueFuturePromise;
};

} // namespace asyncly::test