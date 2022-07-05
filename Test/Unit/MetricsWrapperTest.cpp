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
#include <chrono>
#include <deque>
#include <future>
#include <memory>
#include <set>
#include <sstream>

#include "asyncly/ExecutorTypes.h"
#include "asyncly/future/Future.h"
#include "asyncly/test/FakeFutureTest.h"
#include "boost/optional.hpp"

#include "prometheus/metric_family.h"

#include "gmock/gmock.h"

#include "asyncly/executor/MetricsWrapper.h"
#include "asyncly/executor/ThreadPoolExecutorController.h"
#include "asyncly/test/FakeExecutor.h"
#include "asyncly/test/IFakeExecutor.h"

#include "detail/PrometheusTestHelper.h"

using namespace testing;
using namespace asyncly;

namespace {

enum class PostCallType { kPost, kPostAt, kPostAfter };

inline boost::optional<std::shared_ptr<Cancelable>> postTaskNow(
    const IExecutorPtr& executor,
    PostCallType postType,
    Task&& task,
    const asyncly::clock_type::duration& duration = std::chrono::milliseconds(0))
{
    switch (postType) {
    case PostCallType::kPost:
        executor->post(std::move(task));
        return {};
    case PostCallType::kPostAt:
        return executor->post_at(executor->now() + duration, std::move(task));
    case PostCallType::kPostAfter:
        return executor->post_after(duration, std::move(task));
    }
    throw std::logic_error("enum value not handled in switch");
}
} // namespace

class MetricsWrapperTest : public test::FakeFutureTestBase<::testing::TestWithParam<PostCallType>> {
  public:
    void SetUp() override
    {
        targetCounterValue_ = std::map<PostCallType, std::string>{
            std::make_pair(PostCallType::kPost, "immediate"),
            std::make_pair(PostCallType::kPostAt, "timed"),
            std::make_pair(PostCallType::kPostAfter, "timed")
        };

        registry_ = std::make_shared<prometheus::Registry>();
        metricsExecutor_ = create_metrics_wrapper(get_fake_executor(), "", registry_);
    }

    std::shared_ptr<prometheus::Registry> registry_;
    IExecutorPtr metricsExecutor_;

    std::map<PostCallType, std::string> targetCounterValue_;
};

TEST_F(MetricsWrapperTest, shouldRunPostedTask)
{
    std::promise<void> taskIsRun;
    metricsExecutor_->post([&taskIsRun]() { taskIsRun.set_value(); });

    get_fake_executor()->runTasks();
    taskIsRun.get_future().wait();
}

TEST_F(MetricsWrapperTest, shouldNotAcceptInvalidExecutor)
{
    auto createMetricsWrapper = [this]() { auto e = create_metrics_wrapper({}, "", registry_); };
    EXPECT_THROW(createMetricsWrapper(), std::exception);
}

TEST_F(MetricsWrapperTest, shouldProvideFourMetricFamilies)
{
    EXPECT_GE(4U, registry_->Collect().size());
}

TEST_F(MetricsWrapperTest, shouldProvideMetricFamiliesForQueueAndTimingStatistics)
{
    const auto families = registry_->Collect();

    auto familyNames = std::set<std::string>{};
    std::transform(
        families.begin(),
        families.end(),
        std::inserter(familyNames, familyNames.begin()),
        [](const prometheus::MetricFamily& family) { return family.name; });

    EXPECT_THAT(familyNames, Contains("processed_tasks_total"));
    EXPECT_THAT(familyNames, Contains("currently_enqueued_tasks_total"));
    EXPECT_THAT(familyNames, Contains("task_execution_duration_ns"));
    EXPECT_THAT(familyNames, Contains("task_queueing_delay_ns"));
}

TEST_F(MetricsWrapperTest, shouldProvideImmediateAndTimedMetrics)
{
    const auto families = registry_->Collect();

    for (auto& family : families) {
        if (family.name != "task_execution_duration_ns") {
            EXPECT_THAT(
                family.metric,
                Contains(Field(
                    &prometheus::ClientMetric::label,
                    Contains(AllOf(
                        Field(&prometheus::ClientMetric::Label::name, "type"),
                        Field(&prometheus::ClientMetric::Label::value, "immediate"))))));
            EXPECT_THAT(
                family.metric,
                Contains(Field(
                    &prometheus::ClientMetric::label,
                    Contains(AllOf(
                        Field(&prometheus::ClientMetric::Label::name, "type"),
                        Field(&prometheus::ClientMetric::Label::value, "timed"))))));
        }
    }
}

TEST_P(MetricsWrapperTest, shouldCountQueuedTasks)
{
    const auto numberOfPostedTasks = 100;
    const auto numberOfQueuedTasks = numberOfPostedTasks;
    for (auto i = 0; i < numberOfPostedTasks; i++) {
        postTaskNow(
            metricsExecutor_, GetParam(), [] {}, std::chrono::seconds(i));
    }

    const auto families = registry_->Collect();
    const auto result = detail::grabMetric(
        families,
        prometheus::MetricType::Gauge,
        "currently_enqueued_tasks_total",
        targetCounterValue_[GetParam()]);
    EXPECT_TRUE(result.success) << result.errorMessage;

    EXPECT_DOUBLE_EQ(result.metric.gauge.value, static_cast<double>(numberOfQueuedTasks));
}

TEST_P(MetricsWrapperTest, shouldStartWithZeroProcessedTasks)
{
    const auto families = registry_->Collect();
    const auto result = detail::grabMetric(
        families,
        prometheus::MetricType::Counter,
        "processed_tasks_total",
        targetCounterValue_[GetParam()]);
    EXPECT_TRUE(result.success) << result.errorMessage;

    EXPECT_DOUBLE_EQ(result.metric.counter.value, 0.0);
}

TEST_P(MetricsWrapperTest, shouldStartWithZeroQueuedTasks)
{
    const auto families = registry_->Collect();
    const auto result = detail::grabMetric(
        families,
        prometheus::MetricType::Gauge,
        "currently_enqueued_tasks_total",
        targetCounterValue_[GetParam()]);
    EXPECT_TRUE(result.success) << result.errorMessage;

    EXPECT_DOUBLE_EQ(result.metric.gauge.value, 0.0);
}

TEST_P(MetricsWrapperTest, shouldCountProcessedTasks)
{
    auto numberOfPostedTasks = 100;
    std::promise<void> done;
    for (auto i = 0; i < numberOfPostedTasks; i++) {
        const auto task = [&done, i, numberOfPostedTasks]() {
            // Let us know when the last task is done
            if (i == numberOfPostedTasks - 1) {
                done.set_value();
            }
        };

        postTaskNow(metricsExecutor_, GetParam(), std::move(task));
    }
    get_fake_executor()->advanceClock(asyncly::clock_type::duration::max());
    done.get_future().wait();

    const auto families = registry_->Collect();
    const auto result = detail::grabMetric(
        families,
        prometheus::MetricType::Counter,
        "processed_tasks_total",
        targetCounterValue_[GetParam()]);
    EXPECT_TRUE(result.success) << result.errorMessage;

    EXPECT_DOUBLE_EQ(result.metric.counter.value, static_cast<double>(numberOfPostedTasks));
}

TEST_F(MetricsWrapperTest, shouldNotCountCanceledQueuedTasks)
{
    const auto numberOfPostedTasks = 100;
    auto numberOfTaskToCancel = numberOfPostedTasks / 3;
    const auto numberOfQueuedTasks = numberOfPostedTasks - numberOfTaskToCancel;
    std::vector<std::shared_ptr<Cancelable>> cancelables;
    for (auto i = 0; i < numberOfPostedTasks; i++) {

        cancelables.push_back(
            postTaskNow(
                metricsExecutor_, PostCallType::kPostAt, [] {}, std::chrono::seconds(i))
                .get());
    }

    for (const auto& cancelable : cancelables) {
        cancelable->cancel();
        if (--numberOfTaskToCancel == 0) {
            break;
        }
    }

    const auto families = registry_->Collect();
    const auto result = detail::grabMetric(
        families,
        prometheus::MetricType::Gauge,
        "currently_enqueued_tasks_total",
        targetCounterValue_[PostCallType::kPostAt]);
    EXPECT_TRUE(result.success) << result.errorMessage;

    EXPECT_DOUBLE_EQ(result.metric.gauge.value, static_cast<double>(numberOfQueuedTasks));
}

TEST_P(MetricsWrapperTest, shouldMeasureTaskRuntime)
{
    const auto expectedSummarizedTaskRuntime = 500;
    const auto expectedSampleCount = 1;

    auto task = [fakeExecutor{ get_fake_executor() }, expectedSummarizedTaskRuntime] {
        fakeExecutor->advanceClock(std::chrono::nanoseconds(expectedSummarizedTaskRuntime));
    };

    postTaskNow(metricsExecutor_, GetParam(), std::move(task));
    get_fake_executor()->advanceClock(asyncly::clock_type::duration::max());

    const auto families = registry_->Collect();
    // there is no separation between timed and immediate ("type") tasks for this metric
    // pass empty string as "type" label
    const auto result = detail::grabMetric(
        families, prometheus::MetricType::Histogram, "task_execution_duration_ns", "");
    EXPECT_TRUE(result.success) << result.errorMessage;

    EXPECT_EQ(result.metric.histogram.sample_count, expectedSampleCount);
    EXPECT_DOUBLE_EQ(
        result.metric.histogram.sample_sum, static_cast<double>(expectedSummarizedTaskRuntime));
}

TEST_P(MetricsWrapperTest, shouldMeasureQueueingDelay)
{
    const auto expectedQueingDelayNs = 500;
    const auto actualExpectedQueingDelayNs = (GetParam() == PostCallType::kPost) ? 0 : 500;
    const auto expectedSampleCount = 1;

    postTaskNow(
        metricsExecutor_, GetParam(), [] {}, std::chrono::nanoseconds(expectedQueingDelayNs));
    get_fake_executor()->advanceClock(asyncly::clock_type::duration::max());

    const auto families = registry_->Collect();
    const auto result = detail::grabMetric(
        families,
        prometheus::MetricType::Histogram,
        "task_queueing_delay_ns",
        targetCounterValue_[GetParam()]);
    EXPECT_TRUE(result.success) << result.errorMessage;

    EXPECT_EQ(result.metric.histogram.sample_count, expectedSampleCount);
    EXPECT_DOUBLE_EQ(
        result.metric.histogram.sample_sum, static_cast<double>(actualExpectedQueingDelayNs));
}

//@todo kPostPeriodically needs to be added when MetricsWrapper is fixed
INSTANTIATE_TEST_SUITE_P(
    PostCallType,
    MetricsWrapperTest,
    Values(PostCallType::kPost, PostCallType::kPostAt, PostCallType::kPostAfter));
