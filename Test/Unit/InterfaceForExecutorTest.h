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

#include "asyncly/future/Future.h"
#include "asyncly/observable/Observable.h"

namespace asyncly {

class InterfaceForExecutorTest {
  public:
    virtual ~InterfaceForExecutorTest() = default;
    virtual void method() = 0;
    virtual void methodWithArguments(std::unique_ptr<bool> a, int c) = 0;
    virtual Future<int> methodReturningFuture(bool b) = 0;
    virtual Future<void> methodReturningVoidFuture(bool b) = 0;
    virtual Observable<int> methodReturningObservable(bool b) = 0;
};
}
