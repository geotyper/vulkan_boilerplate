#pragma once

#include "vkexp/core/Module.hpp"

#include <vulkan/vulkan.h>

namespace vkexp {

class ComputeModule final : public Module {
public:
    void onAttach(AppContext& context) override;
    void onUpdate(AppContext& context, const FrameInfo& frame) override;
    void onRender(AppContext& context, const FrameInfo& frame) override;
    void onDetach(AppContext& context) override;

private:
    static constexpr VkFormat outputFormat = VK_FORMAT_R8G8B8A8_UNORM;

    void createOutput(AppContext& context);
    void destroyOutput(AppContext& context);
    void updateDescriptors(AppContext& context);
    [[nodiscard]] std::uint32_t findMemoryType(AppContext& context,
                                               std::uint32_t typeFilter,
                                               VkMemoryPropertyFlags properties) const;

    VkDescriptorSetLayout descriptorSetLayout_{};
    VkDescriptorPool descriptorPool_{};
    VkDescriptorSet descriptorSet_{};
    VkPipelineLayout pipelineLayout_{};
    VkPipeline pipeline_{};
    VkImage outputImage_{};
    VkDeviceMemory outputMemory_{};
    VkImageView outputView_{};
    VkSampler outputSampler_{};
    VkImageLayout outputLayout_{VK_IMAGE_LAYOUT_UNDEFINED};
    std::uint64_t sourceGeneration_{};
};

} // namespace vkexp

