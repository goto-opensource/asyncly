/*
 * Copyright (c) GoTo
 * All Rights Reserved Worldwide.
 *
 * THIS PROGRAM IS CONFIDENTIAL AND PROPRIETARY TO GOTO
 * AND CONSTITUTES A VALUABLE TRADE SECRET.
 */

#pragma once

#include "asyncly/future/Future.h"

#include <string>
#include <type_traits>

namespace asyncly {
template <typename F> auto futurize(F&& f, const std::string& error)
{
    using R = std::remove_cv_t<decltype(f())>;

    try {
        return asyncly::make_ready_future(f());
    } catch (const std::exception& e) {
        return asyncly::make_exceptional_future<R>(error + ": " + e.what());
    } catch (...) {
        return asyncly::make_exceptional_future<R>(error);
    }
}
} // namespace app_server_client
