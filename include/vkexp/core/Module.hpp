#pragma once

#include <vulkan/vulkan.h>

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

struct RenderViewport {
    VkImage image{};
    VkImageView imageView{};
    VkSampler sampler{};
    VkExtent2D extent{960, 540};
    std::uint32_t requestedWidth{960};
    std::uint32_t requestedHeight{540};
    std::uint64_t generation{};
};

struct ComputeOutput {
    VkImageView imageView{};
    VkSampler sampler{};
    VkExtent2D extent{};
    std::uint64_t generation{};
    bool requested{};
    bool ready{};
    int radius{4};
};

struct AppContext {
    Window& window;
    VulkanContext& vulkan;
    Preset& preset;
    RenderViewport& viewport;
    ComputeOutput& blur;
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

