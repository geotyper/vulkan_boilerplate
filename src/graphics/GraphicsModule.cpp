#include "vkexp/graphics/GraphicsModule.hpp"

#include "vkexp/core/VulkanContext.hpp"
#include "vkexp/presets/Preset.hpp"

#include <array>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace vkexp {
namespace {

std::vector<std::byte> readBinary(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to open shader: " + path);
    }
    const auto size = file.tellg();
    if (size <= 0 || size % 4 != 0) {
        throw std::runtime_error("Invalid SPIR-V file: " + path);
    }
    std::vector<std::byte> data(static_cast<std::size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

VkShaderModule loadShader(const VkDevice device, const std::string& path) {
    const auto code = readBinary(path);
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const std::uint32_t*>(code.data());
    VkShaderModule module{};
    if (vkCreateShaderModule(device, &info, nullptr, &module) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create shader module: " + path);
    }
    return module;
}

} // namespace

void GraphicsModule::onAttach(AppContext& context) {
    const VkDevice device = context.vulkan.device();
    const VkShaderModule vertex = loadShader(device, VKEXP_SHADER_DIR "/triangle.vert.spv");
    const VkShaderModule fragment = loadShader(device, VKEXP_SHADER_DIR "/triangle.frag.spv");

    try {
        VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
            throw std::runtime_error("Unable to create graphics pipeline layout");
        }

        const std::array stages{
            VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                            nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertex, "main",
                                            nullptr},
            VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                            nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragment,
                                            "main", nullptr},
        };
        VkPipelineVertexInputStateCreateInfo vertexInput{
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewportState{
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo rasterizer{
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.lineWidth = 1.0F;
        VkPipelineMultisampleStateCreateInfo multisampling{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blending{
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blending.attachmentCount = 1;
        blending.pAttachments = &blendAttachment;
        constexpr std::array dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        const VkFormat colorFormat = context.vulkan.colorFormat();
        VkPipelineRenderingCreateInfo rendering{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        rendering.colorAttachmentCount = 1;
        rendering.pColorAttachmentFormats = &colorFormat;

        VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipelineInfo.pNext = &rendering;
        pipelineInfo.stageCount = static_cast<std::uint32_t>(stages.size());
        pipelineInfo.pStages = stages.data();
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &assembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &blending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout_;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                      &pipeline_) != VK_SUCCESS) {
            throw std::runtime_error("Unable to create graphics pipeline");
        }
    } catch (...) {
        vkDestroyShaderModule(device, fragment, nullptr);
        vkDestroyShaderModule(device, vertex, nullptr);
        throw;
    }
    vkDestroyShaderModule(device, fragment, nullptr);
    vkDestroyShaderModule(device, vertex, nullptr);
}

void GraphicsModule::onUpdate(AppContext&, const FrameInfo&) {}

void GraphicsModule::onRender(AppContext& context, const FrameInfo&) {
    const auto& color = context.preset.clearColor;
    const VkClearColorValue clear{{color.r, color.g, color.b, color.a}};
    context.vulkan.beginColorPass(VK_ATTACHMENT_LOAD_OP_CLEAR, clear);
    if (context.preset.graphicsEnabled) {
        const VkExtent2D extent = context.vulkan.extent();
        const VkViewport viewport{0.0F, 0.0F, static_cast<float>(extent.width),
                                  static_cast<float>(extent.height), 0.0F, 1.0F};
        const VkRect2D scissor{{0, 0}, extent};
        vkCmdSetViewport(context.vulkan.commandBuffer(), 0, 1, &viewport);
        vkCmdSetScissor(context.vulkan.commandBuffer(), 0, 1, &scissor);
        vkCmdBindPipeline(context.vulkan.commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_);
        vkCmdDraw(context.vulkan.commandBuffer(), 3, 1, 0, 0);
    }
    context.vulkan.endColorPass();
}

void GraphicsModule::onDetach(AppContext& context) {
    vkDestroyPipeline(context.vulkan.device(), pipeline_, nullptr);
    vkDestroyPipelineLayout(context.vulkan.device(), pipelineLayout_, nullptr);
    pipeline_ = VK_NULL_HANDLE;
    pipelineLayout_ = VK_NULL_HANDLE;
}

} // namespace vkexp

