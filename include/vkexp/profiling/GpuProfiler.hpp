#pragma once

#include "vkexp/profiling/ProfilerTypes.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>

namespace vkexp {

class VulkanContext;

class GpuProfiler {
public:
    class Scope {
    public:
        Scope(GpuProfiler& profiler, VkCommandBuffer commands, ProfileMetric metric);
        ~Scope();

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope(Scope&& other) noexcept;
        Scope& operator=(Scope&&) = delete;

    private:
        GpuProfiler* profiler_{};
        VkCommandBuffer commands_{};
        ProfileMetric metric_{};
    };

    explicit GpuProfiler(VulkanContext& vulkan);
    ~GpuProfiler();

    GpuProfiler(const GpuProfiler&) = delete;
    GpuProfiler& operator=(const GpuProfiler&) = delete;

    void beginFrame(VkCommandBuffer commands);
    void endFrame(VkCommandBuffer commands);
    [[nodiscard]] Scope scope(VkCommandBuffer commands, ProfileMetric metric) {
        return Scope{*this, commands, metric};
    }

    [[nodiscard]] bool supported() const { return supported_; }
    [[nodiscard]] const TimingSeries& series(ProfileMetric metric) const {
        return series_[metricIndex(metric)];
    }

private:
    friend class Scope;

    static constexpr std::uint32_t queryCount =
        static_cast<std::uint32_t>(profileMetricCount * 2U);

    void writeBegin(VkCommandBuffer commands, ProfileMetric metric);
    void writeEnd(VkCommandBuffer commands, ProfileMetric metric);
    void resolvePendingFrame();

    VkDevice device_{};
    VkQueryPool queryPool_{};
    float timestampPeriodNs_{};
    std::uint32_t timestampValidBits_{};
    std::array<bool, profileMetricCount> currentWritten_{};
    std::array<bool, profileMetricCount> pendingWritten_{};
    std::array<TimingSeries, profileMetricCount> series_{};
    bool supported_{};
    bool pendingFrame_{};
};

} // namespace vkexp
