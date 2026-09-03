#include "vkexp/profiling/CpuProfiler.hpp"

#include <utility>

namespace vkexp {

CpuProfiler::Scope::Scope(CpuProfiler& profiler, const ProfileMetricId metric)
    : profiler_(&profiler), metric_(metric), started_(std::chrono::steady_clock::now()) {}

CpuProfiler::Scope::~Scope() {
    if (profiler_ == nullptr) {
        return;
    }
    const auto elapsed = std::chrono::steady_clock::now() - started_;
    profiler_->record(metric_, std::chrono::duration<double, std::milli>(elapsed).count());
}

CpuProfiler::Scope::Scope(Scope&& other) noexcept
    : profiler_(std::exchange(other.profiler_, nullptr)), metric_(other.metric_),
      started_(other.started_) {}

void CpuProfiler::beginFrame() {
    accumulatedMs_.fill(0.0);
    frameStarted_ = Clock::now();
    frameActive_ = true;
}

void CpuProfiler::endFrame(const ProfileMetricId frameMetric, const std::size_t metricCount) {
    if (!frameActive_) {
        return;
    }
    accumulatedMs_[frameMetric] =
        std::chrono::duration<double, std::milli>(Clock::now() - frameStarted_).count();
    for (std::size_t index = 0; index < metricCount; ++index) {
        if (index != frameMetric && accumulatedMs_[index] == 0.0) {
            continue;
        }
        series_[index].add(accumulatedMs_[index]);
    }
    frameActive_ = false;
}

void CpuProfiler::record(const ProfileMetricId metric, const double milliseconds) {
    if (frameActive_ && metric < maxProfileMetrics) {
        accumulatedMs_[metric] += milliseconds;
    }
}

} // namespace vkexp
