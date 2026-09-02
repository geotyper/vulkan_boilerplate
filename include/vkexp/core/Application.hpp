#pragma once

#include "vkexp/core/Module.hpp"
#include "vkexp/core/VulkanContext.hpp"
#include "vkexp/core/Window.hpp"
#include "vkexp/presets/Preset.hpp"
#include "vkexp/profiling/Profiler.hpp"

#include <memory>
#include <vector>

namespace vkexp {

class Application {
public:
    Application(Preset preset, bool validationEnabled);
    int run();

private:
    Window window_;
    VulkanContext vulkan_;
    Preset preset_;
    RenderViewport viewport_;
    ComputeOutput blur_;
    Profiler profiler_;
    AppContext context_;
    std::vector<std::unique_ptr<Module>> modules_;
};

} // namespace vkexp

