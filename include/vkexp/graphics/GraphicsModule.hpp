#pragma once

#include "vkexp/core/Module.hpp"

#include <vulkan/vulkan.h>

namespace vkexp {

class GraphicsModule final : public Module {
public:
    void onAttach(AppContext& context) override;
    void onUpdate(AppContext& context, const FrameInfo& frame) override;
    void onRender(AppContext& context, const FrameInfo& frame) override;
    void onDetach(AppContext& context) override;

private:
    static constexpr VkFormat targetFormat = VK_FORMAT_R8G8B8A8_UNORM;

    void createRenderTarget(AppContext& context, VkExtent2D extent);
    void destroyRenderTarget(AppContext& context);
    [[nodiscard]] std::uint32_t findMemoryType(AppContext& context,
                                               std::uint32_t typeFilter,
                                               VkMemoryPropertyFlags properties) const;

    VkPipelineLayout pipelineLayout_{};
    VkPipeline pipeline_{};
    VkImage targetImage_{};
    VkDeviceMemory targetMemory_{};
    VkImageView targetView_{};
    VkSampler targetSampler_{};
    VkImageLayout targetLayout_{VK_IMAGE_LAYOUT_UNDEFINED};
};

} // namespace vkexp

