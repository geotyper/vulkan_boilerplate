#include "vkexp/core/Application.hpp"

#include "vkexp/compute/ComputeModule.hpp"
#include "vkexp/graphics/GraphicsModule.hpp"
#include "vkexp/ui/ImGuiModule.hpp"

#include <algorithm>
#include <chrono>
#include <exception>
#include <utility>

namespace vkexp {

Application::Application(Preset preset, const bool validationEnabled)
    : window_(preset.windowWidth, preset.windowHeight, "Vulkan experiment framework"),
      vulkan_(window_, validationEnabled), preset_(std::move(preset)),
      profiler_(vulkan_), context_{window_, vulkan_, preset_, viewport_, blur_, profiler_} {
    modules_.push_back(std::make_unique<GraphicsModule>());
    modules_.push_back(std::make_unique<ComputeModule>());
    modules_.push_back(std::make_unique<ImGuiModule>());
}

int Application::run() {
    std::size_t attached = 0;
    try {
        for (auto& module : modules_) {
            module->onAttach(context_);
            ++attached;
        }

        using Clock = std::chrono::steady_clock;
        const auto started = Clock::now();
        auto previous = started;
        std::uint64_t frameNumber = 0;
        while (!window_.shouldClose()) {
            profiler_.cpu().beginFrame();
            window_.pollEvents();
            const auto now = Clock::now();
            FrameInfo frame{
                std::chrono::duration<float>(now - previous).count(),
                std::chrono::duration<float>(now - started).count(),
                frameNumber++,
            };
            previous = now;
            for (auto& module : modules_) {
                module->onUpdate(context_, frame);
            }
            if (!vulkan_.beginFrame()) {
                profiler_.cpu().endFrame();
                continue;
            }
            profiler_.gpu().beginFrame(vulkan_.commandBuffer());
            for (auto& module : modules_) {
                module->onRender(context_, frame);
            }
            profiler_.gpu().endFrame(vulkan_.commandBuffer());
            vulkan_.endFrame();
            profiler_.cpu().endFrame();
        }
        vulkan_.waitIdle();
    } catch (...) {
        vulkan_.waitIdle();
        for (std::size_t i = attached; i > 0; --i) {
            modules_[i - 1]->onDetach(context_);
        }
        throw;
    }

    for (auto iterator = modules_.rbegin(); iterator != modules_.rend(); ++iterator) {
        (*iterator)->onDetach(context_);
    }
    return 0;
}

} // namespace vkexp

