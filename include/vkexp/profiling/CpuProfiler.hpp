#pragma once

#include "vkexp/profiling/ProfilerTypes.hpp"

#include <array>
#include <chrono>

namespace vkexp {

class CpuProfiler {
public:
    class Scope {
    public:
        Scope(CpuProfiler& profiler, ProfileMetric metric);
        ~Scope();

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope(Scope&& other) noexcept;
        Scope& operator=(Scope&&) = delete;

    private:
        CpuProfiler* profiler_{};
        ProfileMetric metric_{};
        std::chrono::steady_clock::time_point started_{};
    };

    void beginFrame();
    void endFrame();
    [[nodiscard]] Scope scope(ProfileMetric metric) { return Scope{*this, metric}; }
    [[nodiscard]] const TimingSeries& series(ProfileMetric metric) const {
        return series_[metricIndex(metric)];
    }

private:
    friend class Scope;
    void record(ProfileMetric metric, double milliseconds);

    using Clock = std::chrono::steady_clock;
    Clock::time_point frameStarted_{};
    std::array<double, profileMetricCount> accumulatedMs_{};
    std::array<TimingSeries, profileMetricCount> series_{};
    bool frameActive_{};
};

} // namespace vkexp
