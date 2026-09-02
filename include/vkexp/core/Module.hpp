#pragma once

#include <cstdint>

namespace vkexp {

class VulkanContext;
class Window;
struct Preset;

struct FrameInfo {
    float deltaSeconds{};
    float elapsedSeconds{};
    std::uint64_t frameNumber{};
};

struct AppContext {
    Window& window;
    VulkanContext& vulkan;
    Preset& preset;
};

class Module {
public:
    virtual ~Module() = default;

    virtual void onAttach(AppContext& context) = 0;
    virtual void onUpdate(AppContext& context, const FrameInfo& frame) = 0;
    virtual void onRender(AppContext& context, const FrameInfo& frame) = 0;
    virtual void onDetach(AppContext& context) = 0;
};

} // namespace vkexp

