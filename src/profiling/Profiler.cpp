#include "vkexp/profiling/Profiler.hpp"

#include <algorithm>
#include <stdexcept>

namespace vkexp {

Profiler::Profiler(VulkanContext& vulkan) : gpu_(vulkan) { frameMetric_ = registerMetric("Frame"); }

ProfileMetricId Profiler::registerMetric(const std::string_view name) {
    if (name.empty()) {
        throw std::invalid_argument("Profiler metric name cannot be empty");
    }
    const auto found = std::find(metricNames_.begin(), metricNames_.end(), name);
    if (found != metricNames_.end()) {
        return static_cast<ProfileMetricId>(std::distance(metricNames_.begin(), found));
    }
    if (metricNames_.size() >= maxProfileMetrics) {
        throw std::length_error("Profiler metric capacity exceeded");
    }
    const auto metric = static_cast<ProfileMetricId>(metricNames_.size());
    metricNames_.emplace_back(name);
    return metric;
}

} // namespace vkexp
