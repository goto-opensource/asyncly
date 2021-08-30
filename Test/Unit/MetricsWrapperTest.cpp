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
#include <deque>
#include <future>
#include <memory>
#include <set>
#include <sstream>

#include "boost/optional.hpp"

#include "prometheus/metric_family.h"

#include "gmock/gmock.h"

#include "asyncly/executor/MetricsWrapper.h"
#include "asyncly/executor/ThreadPoolExecutorController.h"

#include "detail/PrometheusTestHelper.h"

using namespace testing;
using namespace asyncly;

namespace {
// ASYNCLY-13
/*using SteadyClockTimePoint = clock_type::time_point;
using ZeroTimePoint = std::chrono::time_point<clock_type>;

class FakeClock {
  public:
    static SteadyClockTimePoint now()
    {
        if (!ourTimesToReturn.empty()) {
            const auto ret = ourTimesToReturn.front();
            ourTimesToReturn.pop_front();
            return ret;
        }

        return ZeroTimePoint{};
    };

    static std::deque<SteadyClockTimePoint> ourTimesToReturn;
};
std::deque<SteadyClockTimePoint> FakeClock::ourTimesToReturn{};*/

enum class PostCallType { kPost, kPostAt, kPostAfter };

inline boost::optional<std::shared_ptr<Cancelable>>
postTaskNow(const IExecutorPtr& executor, PostCallType postType, Task&& task)
{
    switch (postType) {
    case PostCallType::kPost:
        executor->post(std::move(task));
        return {};
    case PostCallType::kPostAt:
        return executor->post_at(
            clock_type::now() + std::chrono::milliseconds{ 0 }, std::move(task));
    case PostCallType::kPostAfter:
        return executor->post_after(std::chrono::milliseconds{ 0 }, std::move(task));
    }
    throw std::logic_error("enum value not handled in switch");
}
}

class MetricsWrapperTest : public TestWithParam<PostCallType> {
  public:
    void SetUp() override
    {
        targetCounterValue_ = std::map<PostCallType, std::string>{
            std::make_pair(PostCallType::kPost, "immediate"),
            std::make_pair(PostCallType::kPostAt, "timed"),
            std::make_pair(PostCallType::kPostAfter, "timed")
        };

        executorController_ = ThreadPoolExecutorController::create(1);
        executor_ = executorController_->get_executor();
        registry_ = std::make_shared<prometheus::Registry>();
        metricsExecutor_ = create_metrics_wrapper(executor_, "", registry_);
    }

    IExecutorControllerUPtr executorController_;
    IExecutorPtr executor_;
    std::shared_ptr<prometheus::Registry> registry_;
    IExecutorPtr metricsExecutor_;

    std::map<PostCallType, std::string> targetCounterValue_;
};

TEST_F(MetricsWrapperTest, shouldRunPostedTask)
{
    std::promise<void> taskIsRun;
    metricsExecutor_->post([&taskIsRun]() { taskIsRun.set_value(); });

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

TEST_F(MetricsWrapperTest, shouldCountQueuedTasks)
{
    auto numberOfPostedTasks = 100;
    const auto numberOfQueuedTasks = 0;
    std::promise<void> blocker;
    std::promise<void> blocked;
    auto blockerSharedFuture = blocker.get_future().share();
    std::vector<std::shared_ptr<Cancelable>> cancelables;
    for (auto i = 0; i < numberOfPostedTasks; i++) {
        const auto task = [blockerSharedFuture, &blocked, i]() {
            // Delay first task till test done
            if (i == 0) {
                blocked.set_value();
                blockerSharedFuture.wait();
            }
        };

        cancelables.push_back(
            postTaskNow(metricsExecutor_, PostCallType::kPostAt, std::move(task)).get());
    }

    // Wait for first task to run, it blocks, so we know whats in the queue
    blocked.get_future().wait();
    for (const auto& cancelable : cancelables) {
        cancelable->cancel();
    }

    const auto families = registry_->Collect();
    const auto result = detail::grabMetric(
        families,
        prometheus::MetricType::Gauge,
        "currently_enqueued_tasks_total",
        targetCounterValue_[PostCallType::kPostAt]);
    EXPECT_TRUE(result.success) << result.errorMessage;

    EXPECT_DOUBLE_EQ(result.metric.gauge.value, static_cast<double>(numberOfQueuedTasks));

    // Continue and finalize task execution
    blocker.set_value();
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
    done.get_future().wait();

    for (;;) {
        const auto families = registry_->Collect();
        const auto result = detail::grabMetric(
            families,
            prometheus::MetricType::Counter,
            "processed_tasks_total",
            targetCounterValue_[GetParam()]);
        EXPECT_TRUE(result.success) << result.errorMessage;

        if (result.metric.counter.value != numberOfPostedTasks) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        EXPECT_DOUBLE_EQ(result.metric.counter.value, static_cast<double>(numberOfPostedTasks));
        break;
    }
}

TEST_P(MetricsWrapperTest, shouldNotCountCanceledQueuedTasks)
{
    auto numberOfPostedTasks = 100;
    const auto numberOfQueuedTasks = numberOfPostedTasks - 1;
    std::promise<void> done;
    std::promise<void> blocker;
    std::promise<void> blocked;
    auto blockerSharedFuture = blocker.get_future().share();
    for (auto i = 0; i < numberOfPostedTasks; i++) {
        const auto task = [&done, blockerSharedFuture, &blocked, i, numberOfPostedTasks]() {
            // Delay first task till test done
            if (i == 0) {
                blocked.set_value();
                blockerSharedFuture.wait();
            }
            // Let us know when the last task is done
            if (i == numberOfPostedTasks - 1) {
                done.set_value();
            }
        };

        postTaskNow(metricsExecutor_, GetParam(), std::move(task));
    }

    // Wait for first task to run, it blocks, so we know whats in the queue
    blocked.get_future().wait();

    for (;;) {
        const auto families = registry_->Collect();
        const auto result = detail::grabMetric(
            families,
            prometheus::MetricType::Gauge,
            "currently_enqueued_tasks_total",
            targetCounterValue_[GetParam()]);
        EXPECT_TRUE(result.success) << result.errorMessage;

        if (result.metric.gauge.value != numberOfQueuedTasks) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        EXPECT_DOUBLE_EQ(result.metric.gauge.value, static_cast<double>(numberOfQueuedTasks));
        break;
    }

    // Continue and finalize task execution
    blocker.set_value();
    done.get_future().wait();
}

/*TEST_P(MetricsWrapperTest, DISABLED_shouldMeasureTaskRuntime) // ASYNCLY-13
{
    static const auto expectedSummarizedTaskRuntime = 1000;
    static const auto expectedSampleCount = 1;

    FakeClock::ourTimesToReturn.emplace_back(); // 1) Used for post time point (not
                                                // important here)
    FakeClock::ourTimesToReturn.emplace_back(); // 2) Used for start of task (important)
    static const auto secondTimePoint
        = ZeroTimePoint{} + std::chrono::nanoseconds{ expectedSummarizedTaskRuntime };
    FakeClock::ourTimesToReturn.push_back(
        secondTimePoint); // 3) [synchronizationTask] Used for post time point (not important)
    FakeClock::ourTimesToReturn.push_back(secondTimePoint); // 4) Used for end of task (important)
    FakeClock::ourTimesToReturn.push_back(
        secondTimePoint); // 5) [synchronizationTask] Used for start of task (not important)
    FakeClock::ourTimesToReturn.push_back(
        secondTimePoint); // 6) [synchronizationTask] Used for end of task (not important)

    // Run a task to measure "task runtime" (block it till we post a second task)
    std::promise<void> measuredTaskIsRunning;
    std::promise<void> blockMeasuredTask;
    auto measuredTask
        = [&measuredTaskIsRunning, blockMeasuredTaskFuture{ blockMeasuredTask.get_future() }]() {
              measuredTaskIsRunning.set_value();
              blockMeasuredTaskFuture.wait();
          };
    postTaskNow(metricsExecutor_, GetParam(), std::move(measuredTask));

    // Wait for it to run, THEN run a second task, THEN ensure second task runs, THEN block it so we
    // only have one measurement sample for the measuredTask
    measuredTaskIsRunning.get_future().wait();
    std::promise<void> synchronizationTaskRuns;
    std::promise<void> blockSynchronizationTask;
    auto synchronizationTask
        = [&synchronizationTaskRuns,
           blockSynchronizationTaskFuture{ blockSynchronizationTask.get_future() }]() {
              synchronizationTaskRuns.set_value();
              blockSynchronizationTaskFuture.wait();
          };
    postTaskNow(metricsExecutor_, GetParam(), std::move(synchronizationTask));
    blockMeasuredTask.set_value();
    synchronizationTaskRuns.get_future().wait();

    for (;;) {
        const auto families = registry_->Collect();
        const auto result = detail::grabMetric(
            families,
            prometheus::MetricType::Histogram,
            "task_execution_duration_ns",
            targetCounterValue_[GetParam()]);
        EXPECT_TRUE(result.success) << result.errorMessage;

        if (result.metric.histogram.sample_count != expectedSampleCount
            || result.metric.histogram.sample_sum != expectedSummarizedTaskRuntime) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        EXPECT_EQ(result.metric.histogram.sample_count, expectedSampleCount);
        EXPECT_DOUBLE_EQ(
            result.metric.histogram.sample_sum, static_cast<double>(expectedSummarizedTaskRuntime));
        break;
    }

    blockSynchronizationTask.set_value();
}

TEST_P(MetricsWrapperTest, DISABLED_shouldMeasureQueueingDelay)
{
    static const auto expectedQueingDelay = 1000;
    static const auto expectedSampleCount = 1;

    FakeClock::ourTimesToReturn.emplace_back(); // 1) Used for post time point (important)
    FakeClock::ourTimesToReturn.push_back(
        ZeroTimePoint{}
        + std::chrono::nanoseconds{ expectedQueingDelay }); // 2) Used for start of task (important)
    FakeClock::ourTimesToReturn.push_back(
        ZeroTimePoint{}
        + std::chrono::nanoseconds{
            expectedQueingDelay }); // 3) Used for end of task (not important)

    std::promise<void> done;
    const auto task = [&done]() {
        // Let us know when the task is done
        done.set_value();
    };

    postTaskNow(metricsExecutor_, GetParam(), std::move(task));
    done.get_future().wait();

    for (;;) {
        const auto families = registry_->Collect();
        const auto result = detail::grabMetric(
            families,
            prometheus::MetricType::Histogram,
            "task_queueing_delay_ns",
            targetCounterValue_[GetParam()]);
        EXPECT_TRUE(result.success) << result.errorMessage;

        if (result.metric.histogram.sample_count != expectedSampleCount
            || result.metric.histogram.sample_sum != expectedQueingDelay) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        EXPECT_EQ(result.metric.histogram.sample_count, expectedSampleCount);
        EXPECT_DOUBLE_EQ(
            result.metric.histogram.sample_sum, static_cast<double>(expectedQueingDelay));
        break;
    }
}*/

//@todo kPostPeriodically needs to be added when MetricsWrapper is fixed
INSTANTIATE_TEST_SUITE_P(
    PostCallType,
    MetricsWrapperTest,
    Values(PostCallType::kPost, PostCallType::kPostAt, PostCallType::kPostAfter));
