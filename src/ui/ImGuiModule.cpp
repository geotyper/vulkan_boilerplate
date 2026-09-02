#include "vkexp/ui/ImGuiModule.hpp"

#include "vkexp/core/VulkanContext.hpp"
#include "vkexp/core/Window.hpp"
#include "vkexp/presets/Preset.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <array>
#include <stdexcept>

namespace vkexp {

void ImGuiModule::onAttach(AppContext& context) {
    constexpr std::array poolSizes{
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, 128},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 128},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 128},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 128},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 128},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 128},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 128},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 128},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 128},
    };
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 128;
    poolInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    if (vkCreateDescriptorPool(context.vulkan.device(), &poolInfo, nullptr, &descriptorPool_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Unable to create ImGui descriptor pool");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    if (!ImGui_ImplGlfw_InitForVulkan(context.window.handle(), true)) {
        throw std::runtime_error("Unable to initialize the ImGui GLFW backend");
    }

    const VkFormat colorFormat = context.vulkan.colorFormat();
    VkPipelineRenderingCreateInfo renderingInfo{
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &colorFormat;
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = context.vulkan.instance();
    initInfo.PhysicalDevice = context.vulkan.physicalDevice();
    initInfo.Device = context.vulkan.device();
    initInfo.QueueFamily = context.vulkan.graphicsQueueFamily();
    initInfo.Queue = context.vulkan.graphicsQueue();
    initInfo.DescriptorPool = descriptorPool_;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = context.vulkan.imageCount();
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo = renderingInfo;
    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        throw std::runtime_error("Unable to initialize the ImGui Vulkan backend");
    }
}

void ImGuiModule::onUpdate(AppContext& context, const FrameInfo& frame) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Vulkan experiments");
    ImGui::Text("Preset: %s", context.preset.name.c_str());
    ImGui::Text("Frame: %llu", static_cast<unsigned long long>(frame.frameNumber));
    ImGui::Text("%.2f ms (%.1f FPS)", frame.deltaSeconds * 1000.0F,
                frame.deltaSeconds > 0.0F ? 1.0F / frame.deltaSeconds : 0.0F);
    ImGui::Separator();
    ImGui::Checkbox("Graphics pipeline", &context.preset.graphicsEnabled);
    ImGui::Checkbox("Compute pipeline", &context.preset.computeEnabled);
    int computeGroups = static_cast<int>(context.preset.computeGroups);
    if (ImGui::SliderInt("Compute groups", &computeGroups, 1, 256)) {
        context.preset.computeGroups = static_cast<unsigned int>(computeGroups);
    }
    ImGui::ColorEdit3("Clear color", &context.preset.clearColor.x);
    ImGui::Checkbox("ImGui demo", &context.preset.showDemoWindow);
    ImGui::End();
    if (context.preset.showDemoWindow) {
        ImGui::ShowDemoWindow(&context.preset.showDemoWindow);
    }
    ImGui::Render();
}

void ImGuiModule::onRender(AppContext& context, const FrameInfo&) {
    const VkClearColorValue unusedClear{};
    context.vulkan.beginColorPass(VK_ATTACHMENT_LOAD_OP_LOAD, unusedClear);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), context.vulkan.commandBuffer());
    context.vulkan.endColorPass();
}

void ImGuiModule::onDetach(AppContext& context) {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(context.vulkan.device(), descriptorPool_, nullptr);
    descriptorPool_ = VK_NULL_HANDLE;
}

} // namespace vkexp

