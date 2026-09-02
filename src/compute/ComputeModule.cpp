#include "vkexp/compute/ComputeModule.hpp"

#include "vkexp/core/VulkanContext.hpp"
#include "vkexp/presets/Preset.hpp"

#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace vkexp {
namespace {

VkShaderModule loadComputeShader(const VkDevice device) {
    const std::string path = VKEXP_SHADER_DIR "/experiment.comp.spv";
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to open compute shader: " + path);
    }
    const auto size = file.tellg();
    if (size <= 0 || size % 4 != 0) {
        throw std::runtime_error("Invalid compute SPIR-V: " + path);
    }
    std::vector<std::byte> code(static_cast<std::size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(code.data()), size);
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const std::uint32_t*>(code.data());
    VkShaderModule shader{};
    if (vkCreateShaderModule(device, &info, nullptr, &shader) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create compute shader module");
    }
    return shader;
}

} // namespace

void ComputeModule::onAttach(AppContext& context) {
    const VkDevice device = context.vulkan.device();
    const VkShaderModule shader = loadComputeShader(device);
    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        vkDestroyShaderModule(device, shader, nullptr);
        throw std::runtime_error("Unable to create compute pipeline layout");
    }
    VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = shader;
    stage.pName = "main";
    VkComputePipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipelineInfo.stage = stage;
    pipelineInfo.layout = pipelineLayout_;
    const VkResult result =
        vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_);
    vkDestroyShaderModule(device, shader, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Unable to create compute pipeline");
    }
}

void ComputeModule::onUpdate(AppContext&, const FrameInfo&) {}

void ComputeModule::onRender(AppContext& context, const FrameInfo&) {
    if (!context.preset.computeEnabled) {
        return;
    }
    vkCmdBindPipeline(context.vulkan.commandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
    vkCmdDispatch(context.vulkan.commandBuffer(), context.preset.computeGroups, 1, 1);
}

void ComputeModule::onDetach(AppContext& context) {
    vkDestroyPipeline(context.vulkan.device(), pipeline_, nullptr);
    vkDestroyPipelineLayout(context.vulkan.device(), pipelineLayout_, nullptr);
    pipeline_ = VK_NULL_HANDLE;
    pipelineLayout_ = VK_NULL_HANDLE;
}

} // namespace vkexp

