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

#include "asyncly/future/Future.h"
#include "asyncly/future/Split.h"
#include "asyncly/future/WhenAny.h"

namespace asyncly {

namespace {
template <typename T> struct TimeoutVisitor;
}

/// Timeout is thrown when a timeout augmented future times out.
struct Timeout : public std::exception {
    const char* what() const noexcept override
    {
        return "Timeout";
    }
};

/// add_timeout augments the future passed to it by rejecting the future after a specified
/// amount of time. The error the future is rejected with is asyncly::Timeout.
/// add_timeout consumes the future passed to it.
///
/// Example usage:
///
/// auto connectionFuture = server->connect();
/// add_timeout(std::chrono::seconds(15), std::move(connectionFuture))
///           .then([](auto connection) {
///                // do something with the connection
///           })
///           .catch_error([](auto error) {
///                try {
///                   std::rethrow_exception(error);
///                } catch (asyncly::Timeout) {
///                   // handle timeout
///                } catch (std::exception) {
///                   // handle other connection errors
///                }
///           });

template <typename T, typename Duration>
Future<T> add_timeout(Duration duration, Future<T>&& future)
{
    static_assert(
        !std::is_same<T, Timeout>::value,
        "You cannot use this function with Future<Timeout>, it doesn't make any sense to try that");
    auto timeoutReject = make_lazy_future<Timeout>();
    auto timeoutFuture = std::get<0>(timeoutReject);
    auto timeoutPromise = std::get<1>(timeoutReject);

    auto cancelable = asyncly::this_thread::get_current_executor()->post_after(
        duration,
        [promise = std::move(timeoutPromise)]() mutable { promise.set_exception(Timeout{}); });

    // we need to cancel the timer in case the future is rejected, but we still want the
    // caller to handle errors independently. That's why we split the future and handle
    // the error on the split-off part.
    auto splitFuture = asyncly::split(std::move(future));
    auto returnedFuture = std::get<0>(splitFuture);
    auto errorAugmentedFuture = std::get<1>(splitFuture);
    errorAugmentedFuture.catch_error([cancelable](auto) { cancelable->cancel(); });

    auto combinedFuture = when_any(std::move(returnedFuture), std::move(timeoutFuture))
                              .then([cancelable](auto variant) {
                                  cancelable->cancel();
                                  return boost::variant2::visit(TimeoutVisitor<T>(), variant);
                              });

    return combinedFuture;
}

namespace {
template <typename T> struct TimeoutVisitor {
    T operator()(T value) const
    {
        return value;
    }

    T operator()(Timeout) const
    {
        throw std::runtime_error("Resolved timeout future, this should never happen: it "
                                 "should always be rejected");
    }
};

template <> struct TimeoutVisitor<void> {
    void operator()(boost::none_t) const
    {
    }

    void operator()(Timeout) const
    {
        throw std::runtime_error("Resolved timeout future, this should never happen: it "
                                 "should always be rejected");
    }
};
}
}
