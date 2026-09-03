#pragma once

#include "vkexp/profiling/ProfilerTypes.hpp"

#include <array>
#include <chrono>

namespace vkexp {

class CpuProfiler {
public:
    class Scope {
    public:
        Scope(CpuProfiler& profiler, ProfileMetricId metric);
        ~Scope();

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope(Scope&& other) noexcept;
        Scope& operator=(Scope&&) = delete;

    private:
        CpuProfiler* profiler_{};
        ProfileMetricId metric_{invalidProfileMetric};
        std::chrono::steady_clock::time_point started_{};
    };

    void beginFrame();
    void endFrame(ProfileMetricId frameMetric, std::size_t metricCount);
    [[nodiscard]] Scope scope(ProfileMetricId metric) { return Scope{*this, metric}; }
    [[nodiscard]] const TimingSeries& series(ProfileMetricId metric) const {
        return series_[metric];
    }

private:
    friend class Scope;
    void record(ProfileMetricId metric, double milliseconds);

    using Clock = std::chrono::steady_clock;
    Clock::time_point frameStarted_{};
    std::array<double, maxProfileMetrics> accumulatedMs_{};
    std::array<TimingSeries, maxProfileMetrics> series_{};
    bool frameActive_{};
};

} // namespace vkexp
