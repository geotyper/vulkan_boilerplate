#pragma once

#include "vkexp/core/Module.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

namespace vkexp {

class ImGuiModule final : public Module {
public:
    void onAttach(AppContext& context) override;
    void onUpdate(AppContext& context, const FrameInfo& frame) override;
    void onRender(AppContext& context, const FrameInfo& frame) override;
    void onDetach(AppContext& context) override;

private:
    void syncViewportTextures(AppContext& context);

    VkDescriptorPool descriptorPool_{};
    VkDescriptorSet viewportDescriptor_{};
    VkDescriptorSet blurDescriptor_{};
    std::uint64_t viewportGeneration_{};
    std::uint64_t blurGeneration_{};
    std::string iniPath_;
};

} // namespace vkexp

