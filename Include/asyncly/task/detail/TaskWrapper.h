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

#include "TaskConcept.h"

#include <type_traits>
#include <utility>

namespace asyncly {
namespace detail {

// preserve validation checking for closures that support it, ie
// std::function<void()> bound to a shared_ptr
template <typename T, typename U = T>
typename std::enable_if<std::is_convertible<U, bool>::value, bool>::type
checkClosure(const T& closure)
{
    return closure;
}

// assume valid closures if we cant check
template <typename T, typename U = T>
typename std::enable_if<!std::is_convertible<U, bool>::value, bool>::type checkClosure(const T&)
{
    return true;
}

template <typename T> struct TaskWrapper : public TaskConcept {
    TaskWrapper(T&& closure)
        : closure_(std::move(closure))
    {
        using R = typename std::result_of<T()>::type;
        static_assert(
            std::is_same<R, void>::value, "Posted function objects can not return values!");
    }

    TaskWrapper(const T& closure)
        : closure_(closure)
    {
        using R = typename std::result_of<T()>::type;
        static_assert(
            std::is_same<R, void>::value, "Posted function objects can not return values!");
    }

    void run() override
    {
        closure_();
    }

    operator bool() const override
    {
        return checkClosure(closure_);
    }

    T closure_;
};
}
}
