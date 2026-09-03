#pragma once

#include "vkexp/profiling/CpuProfiler.hpp"
#include "vkexp/profiling/GpuProfiler.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace vkexp {

class VulkanContext;

class Profiler {
public:
    explicit Profiler(VulkanContext& vulkan);

    [[nodiscard]] ProfileMetricId registerMetric(std::string_view name);
    [[nodiscard]] std::span<const std::string> metricNames() const { return metricNames_; }

    [[nodiscard]] CpuProfiler& cpu() { return cpu_; }
    [[nodiscard]] const CpuProfiler& cpu() const { return cpu_; }
    [[nodiscard]] GpuProfiler& gpu() { return gpu_; }
    [[nodiscard]] const GpuProfiler& gpu() const { return gpu_; }

    void beginCpuFrame() { cpu_.beginFrame(); }
    void endCpuFrame() { cpu_.endFrame(frameMetric_, metricNames_.size()); }
    void beginGpuFrame(VkCommandBuffer commands) { gpu_.beginFrame(commands, frameMetric_); }
    void endGpuFrame(VkCommandBuffer commands) { gpu_.endFrame(commands, frameMetric_); }

private:
    CpuProfiler cpu_;
    GpuProfiler gpu_;
    std::vector<std::string> metricNames_;
    ProfileMetricId frameMetric_{invalidProfileMetric};
};

} // namespace vkexp
