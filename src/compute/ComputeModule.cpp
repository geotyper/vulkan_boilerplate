#include "vkexp/compute/ComputeModule.hpp"

#include "vkexp/core/VulkanContext.hpp"
#include "vkexp/presets/Preset.hpp"
#include "vkexp/profiling/Profiler.hpp"

#include <array>
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

    std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[1] = bindings[0];
    bindings[1].binding = 1;
    VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptorLayoutInfo.bindingCount = static_cast<std::uint32_t>(bindings.size());
    descriptorLayoutInfo.pBindings = bindings.data();
    if (vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr,
                                    &descriptorSetLayout_) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create blur descriptor set layout");
    }

    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2};
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create blur descriptor pool");
    }
    VkDescriptorSetAllocateInfo descriptorAllocation{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    descriptorAllocation.descriptorPool = descriptorPool_;
    descriptorAllocation.descriptorSetCount = 1;
    descriptorAllocation.pSetLayouts = &descriptorSetLayout_;
    if (vkAllocateDescriptorSets(device, &descriptorAllocation, &descriptorSet_) != VK_SUCCESS) {
        throw std::runtime_error("Unable to allocate blur descriptor set");
    }

    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstant.size = sizeof(int);
    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout_;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstant;
    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create compute pipeline layout");
    }

    const VkShaderModule shader = loadComputeShader(device);
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
    createOutput(context);
    updateDescriptors(context);
    sourceGeneration_ = context.viewport.generation;
}

std::uint32_t ComputeModule::findMemoryType(AppContext& context,
                                            const std::uint32_t typeFilter,
                                            const VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(context.vulkan.physicalDevice(), &memoryProperties);
    for (std::uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        const bool supported = (typeFilter & (1U << index)) != 0U;
        const bool matches =
            (memoryProperties.memoryTypes[index].propertyFlags & properties) == properties;
        if (supported && matches) {
            return index;
        }
    }
    throw std::runtime_error("Unable to find suitable memory for the blur image");
}

void ComputeModule::createOutput(AppContext& context) {
    const VkDevice device = context.vulkan.device();
    const VkExtent2D extent = context.viewport.extent;
    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = outputFormat;
    imageInfo.extent = {extent.width, extent.height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(device, &imageInfo, nullptr, &outputImage_) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create blur output image");
    }

    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device, outputImage_, &requirements);
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex =
        findMemoryType(context, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device, &allocation, nullptr, &outputMemory_) != VK_SUCCESS ||
        vkBindImageMemory(device, outputImage_, outputMemory_, 0) != VK_SUCCESS) {
        destroyOutput(context);
        throw std::runtime_error("Unable to allocate blur output memory");
    }

    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = outputImage_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = outputFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, nullptr, &outputView_) != VK_SUCCESS) {
        destroyOutput(context);
        throw std::runtime_error("Unable to create blur output image view");
    }

    VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = 1.0F;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &outputSampler_) != VK_SUCCESS) {
        destroyOutput(context);
        throw std::runtime_error("Unable to create blur output sampler");
    }

    outputLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    context.blur.imageView = outputView_;
    context.blur.sampler = outputSampler_;
    context.blur.extent = extent;
    context.blur.ready = false;
    ++context.blur.generation;
}

void ComputeModule::destroyOutput(AppContext& context) {
    const VkDevice device = context.vulkan.device();
    context.blur.imageView = VK_NULL_HANDLE;
    context.blur.sampler = VK_NULL_HANDLE;
    context.blur.ready = false;
    if (outputSampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device, outputSampler_, nullptr);
    }
    if (outputView_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device, outputView_, nullptr);
    }
    if (outputImage_ != VK_NULL_HANDLE) {
        vkDestroyImage(device, outputImage_, nullptr);
    }
    if (outputMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device, outputMemory_, nullptr);
    }
    outputSampler_ = VK_NULL_HANDLE;
    outputView_ = VK_NULL_HANDLE;
    outputImage_ = VK_NULL_HANDLE;
    outputMemory_ = VK_NULL_HANDLE;
    outputLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
}

void ComputeModule::updateDescriptors(AppContext& context) {
    const VkDescriptorImageInfo sourceInfo{
        VK_NULL_HANDLE, context.viewport.imageView, VK_IMAGE_LAYOUT_GENERAL};
    const VkDescriptorImageInfo outputInfo{
        VK_NULL_HANDLE, outputView_, VK_IMAGE_LAYOUT_GENERAL};
    std::array<VkWriteDescriptorSet, 2> writes{};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = descriptorSet_;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[0].pImageInfo = &sourceInfo;
    writes[1] = writes[0];
    writes[1].dstBinding = 1;
    writes[1].pImageInfo = &outputInfo;
    vkUpdateDescriptorSets(context.vulkan.device(), static_cast<std::uint32_t>(writes.size()),
                           writes.data(), 0, nullptr);
}

void ComputeModule::onUpdate(AppContext& context, const FrameInfo&) {
    if (sourceGeneration_ == context.viewport.generation) {
        return;
    }
    context.vulkan.waitIdle();
    destroyOutput(context);
    createOutput(context);
    updateDescriptors(context);
    sourceGeneration_ = context.viewport.generation;
}

void ComputeModule::onRender(AppContext& context, const FrameInfo&) {
    if (!context.preset.computeEnabled) {
        context.blur.requested = false;
        return;
    }
    if (!context.blur.requested) {
        return;
    }

    auto cpuScope = context.profiler.cpu().scope(ProfileMetric::ComputeBlur);
    auto gpuScope =
        context.profiler.gpu().scope(context.vulkan.commandBuffer(), ProfileMetric::ComputeBlur);
    const VkCommandBuffer commands = context.vulkan.commandBuffer();
    std::array<VkImageMemoryBarrier2, 2> toCompute{};
    for (auto& barrier : toCompute) {
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    }
    toCompute[0].srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    toCompute[0].srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    toCompute[0].dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
    toCompute[0].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toCompute[0].image = context.viewport.image;

    toCompute[1].srcStageMask = outputLayout_ == VK_IMAGE_LAYOUT_UNDEFINED
                                    ? VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT
                                    : VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    toCompute[1].srcAccessMask = outputLayout_ == VK_IMAGE_LAYOUT_UNDEFINED
                                     ? 0
                                     : VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    toCompute[1].dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
    toCompute[1].oldLayout = outputLayout_;
    toCompute[1].image = outputImage_;

    VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.imageMemoryBarrierCount = static_cast<std::uint32_t>(toCompute.size());
    dependency.pImageMemoryBarriers = toCompute.data();
    vkCmdPipelineBarrier2(commands, &dependency);

    vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
    vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout_, 0, 1,
                            &descriptorSet_, 0, nullptr);
    vkCmdPushConstants(commands, pipelineLayout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int),
                       &context.blur.radius);
    vkCmdDispatch(commands, (context.blur.extent.width + 7U) / 8U,
                  (context.blur.extent.height + 7U) / 8U, 1);

    std::array<VkImageMemoryBarrier2, 2> toSampled{};
    for (auto& barrier : toSampled) {
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
    }
    toSampled[0].srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
    toSampled[0].image = context.viewport.image;
    toSampled[1].srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
    toSampled[1].image = outputImage_;
    dependency.imageMemoryBarrierCount = static_cast<std::uint32_t>(toSampled.size());
    dependency.pImageMemoryBarriers = toSampled.data();
    vkCmdPipelineBarrier2(commands, &dependency);

    outputLayout_ = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    context.blur.requested = false;
    context.blur.ready = true;
}

void ComputeModule::onDetach(AppContext& context) {
    destroyOutput(context);
    vkDestroyPipeline(context.vulkan.device(), pipeline_, nullptr);
    vkDestroyPipelineLayout(context.vulkan.device(), pipelineLayout_, nullptr);
    vkDestroyDescriptorPool(context.vulkan.device(), descriptorPool_, nullptr);
    vkDestroyDescriptorSetLayout(context.vulkan.device(), descriptorSetLayout_, nullptr);
    pipeline_ = VK_NULL_HANDLE;
    pipelineLayout_ = VK_NULL_HANDLE;
    descriptorPool_ = VK_NULL_HANDLE;
    descriptorSetLayout_ = VK_NULL_HANDLE;
    descriptorSet_ = VK_NULL_HANDLE;
}

} // namespace vkexp

