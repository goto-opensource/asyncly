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

#include <algorithm>
#include <sstream>
#include <string>

#include "prometheus/metric_family.h"
#include "prometheus/metric_type.h"

#include "PrometheusTestHelper.h"

namespace asyncly::detail {

MetricSearchResult grabMetric(
    const std::vector<prometheus::MetricFamily>& families,
    prometheus::MetricType type,
    const std::string& familyName,
    const std::string& labelValue)
{
    const auto processedTaskFamily = std::find_if(
        families.begin(), families.end(), [familyName](const prometheus::MetricFamily& family) {
            return family.name == familyName;
        });

    if (processedTaskFamily == families.end()) {
        return { false,
                 prometheus::ClientMetric{},
                 "==> No family named " + familyName + " found" };
    }
    if (type != processedTaskFamily->type) {
        std::stringstream reason;
        reason << "==> Wrong metric family type! Expected " << static_cast<int>(type) << " got "
               << static_cast<int>(processedTaskFamily->type);
        return { false, prometheus::ClientMetric{}, reason.str() };
    }

    const auto metric = std::find_if(
        processedTaskFamily->metric.begin(),
        processedTaskFamily->metric.end(),
        [labelValue](const prometheus::ClientMetric& metric) {
            for (const auto& label : metric.label) {
                if (label.name == "type" && label.value == labelValue)
                    return true;
            }
            return false;
        });
    if (metric == processedTaskFamily->metric.end()) {
        return { false,
                 prometheus::ClientMetric{},
                 "==> No metric with label=" + labelValue
                     + " found (for metric-family=" + familyName + ")" };
    }
    return { true, *metric, "" };
}
} // namespace asyncly::detail
