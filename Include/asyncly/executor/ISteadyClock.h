/*
 * Copyright (c) 2018 LogMeIn
 * All Rights Reserved Worldwide.
 *
 * THIS PROGRAM IS CONFIDENTIAL AND PROPRIETARY TO LOGMEIN
 * AND CONSTITUTES A VALUABLE TRADE SECRET.
 */

#pragma once

#include <chrono>
#include <memory>

namespace asyncly {

class ISteadyClock {
  public:
    virtual ~ISteadyClock() = default;

    virtual std::chrono::steady_clock::time_point now() = 0;
};

using ISteadyClockPtr = std::shared_ptr<ISteadyClock>;
using ISteadyClockUPtr = std::unique_ptr<ISteadyClock>;

} // namespace asyncly
