#pragma once

#include "vkexp/profiling/CpuProfiler.hpp"
#include "vkexp/profiling/GpuProfiler.hpp"

namespace vkexp {

class VulkanContext;

class Profiler {
public:
    explicit Profiler(VulkanContext& vulkan) : gpu_(vulkan) {}

    [[nodiscard]] CpuProfiler& cpu() { return cpu_; }
    [[nodiscard]] const CpuProfiler& cpu() const { return cpu_; }
    [[nodiscard]] GpuProfiler& gpu() { return gpu_; }
    [[nodiscard]] const GpuProfiler& gpu() const { return gpu_; }

private:
    CpuProfiler cpu_;
    GpuProfiler gpu_;
};

} // namespace vkexp
