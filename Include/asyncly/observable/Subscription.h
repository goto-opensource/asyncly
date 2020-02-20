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

namespace asyncly {

/// Subscription represents the contract started when calling `subscribe` on an
/// `Observable`. It can be used to signal that no further callbacks passed to `subscribe`
/// should be invoked by canceling the subscription.
class Subscription {
  public:
    class Unsubscribable {
      public:
        virtual ~Unsubscribable() = default;
        virtual void cancel() = 0;
    };

  public:
    Subscription(const std::shared_ptr<Unsubscribable>& unsubscribable)
        : impl{ unsubscribable }
    {
    }

    void cancel()
    {
        impl->cancel();
    }

  private:
    const std::shared_ptr<Unsubscribable> impl;
};
}
