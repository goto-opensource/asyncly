/*
 * Copyright (c) LogMeIn
 * All Rights Reserved Worldwide.
 *
 * THIS PROGRAM IS CONFIDENTIAL AND PROPRIETARY TO LOGMEIN
 * AND CONSTITUTES A VALUABLE TRADE SECRET.
 */

#pragma once

#include "asyncly/executor/IStrand.h"

#include <future>

namespace asyncly {
namespace test {

inline void postAndWait(
    const asyncly::IStrandPtr& strand,
    const asyncly::clock_type::duration& timeout,
    const std::function<void()>& task)
{
    std::promise<void> synchronized;
    auto synchronize = [&synchronized, task]() {
        task();
        synchronized.set_value();
    };

    strand->post(synchronize);

    if (synchronized.get_future().wait_for(timeout) == std::future_status::timeout) {
        throw std::runtime_error("Timeout waiting for Task execution");
    }
}

template <typename T>
T postWaitGet(
    const asyncly::IStrandPtr& strand,
    const asyncly::clock_type::duration& timeout,
    const std::function<T()>& task)
{
    std::promise<T> synchronized;
    auto synchronize = [&synchronized, task]() { synchronized.set_value(task()); };

    strand->post(synchronize);

    auto future = synchronized.get_future();
    if (future.wait_for(timeout) == std::future_status::timeout) {
        throw std::runtime_error("Timeout waiting for Task execution");
    }
    return future.get();
}

}
}
