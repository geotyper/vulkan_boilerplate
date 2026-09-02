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
    VkPipelineLayout pipelineLayout_{};
    VkPipeline pipeline_{};
};

} // namespace vkexp

