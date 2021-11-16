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
#include <functional>
#include <vector>

namespace asyncly::detail {

// PriorityQueue implements a priority queue with
// move-capable pop()
template <typename T, typename Container = std::vector<T>, typename Compare = std::less<T>>
class PriorityQueue {
  public:
    void push(T element)
    {
        container.push_back(std::move(element));
        std::push_heap(container.begin(), container.end(), compare);
    }

    T pop()
    {
        std::pop_heap(container.begin(), container.end(), compare);
        auto res = std::move(container.back());
        container.pop_back();
        return std::move(res);
    }

    bool empty() const
    {
        return container.empty();
    }

    const T& peek() const
    {
        return container.front();
    }

    const T& peekBack() const
    {
        return *std::min_element(container.begin(), container.end(), compare);
    }

    void clear()
    {
        container.clear();
    }

  private:
    Container container;
    Compare compare;
};
} // namespace asyncly::detail
