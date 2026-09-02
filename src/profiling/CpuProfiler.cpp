#include "vkexp/profiling/CpuProfiler.hpp"

#include <utility>

namespace vkexp {

CpuProfiler::Scope::Scope(CpuProfiler& profiler, const ProfileMetric metric)
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

void CpuProfiler::endFrame() {
    if (!frameActive_) {
        return;
    }
    accumulatedMs_[metricIndex(ProfileMetric::Frame)] =
        std::chrono::duration<double, std::milli>(Clock::now() - frameStarted_).count();
    for (std::size_t index = 0; index < profileMetricCount; ++index) {
        if (index == metricIndex(ProfileMetric::ComputeBlur) &&
            accumulatedMs_[index] == 0.0) {
            continue;
        }
        series_[index].add(accumulatedMs_[index]);
    }
    frameActive_ = false;
}

void CpuProfiler::record(const ProfileMetric metric, const double milliseconds) {
    if (frameActive_ && metric != ProfileMetric::Frame) {
        accumulatedMs_[metricIndex(metric)] += milliseconds;
    }
}

} // namespace vkexp
