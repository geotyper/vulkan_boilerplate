#include "vkexp/ui/ImGuiModule.hpp"

#include "vkexp/core/VulkanContext.hpp"
#include "vkexp/core/Window.hpp"
#include "vkexp/presets/Preset.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <array>
#include <cmath>
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
    iniPath_ = VKEXP_IMGUI_INI;
    ImGui::GetIO().IniFilename = iniPath_.c_str();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
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
    syncViewportTextures(context);
}

void ImGuiModule::syncViewportTextures(AppContext& context) {
    if (viewportGeneration_ != context.viewport.generation) {
        if (viewportDescriptor_ != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(viewportDescriptor_);
        }
        viewportDescriptor_ = ImGui_ImplVulkan_AddTexture(
            context.viewport.sampler, context.viewport.imageView,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        viewportGeneration_ = context.viewport.generation;
    }
    if (blurGeneration_ != context.blur.generation) {
        if (blurDescriptor_ != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(blurDescriptor_);
        }
        blurDescriptor_ = ImGui_ImplVulkan_AddTexture(
            context.blur.sampler, context.blur.imageView,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        blurGeneration_ = context.blur.generation;
    }
}

void ImGuiModule::onUpdate(AppContext& context, const FrameInfo& frame) {
    syncViewportTextures(context);
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(20.0F, 20.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320.0F, 250.0F), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls");
    ImGui::Text("Preset: %s", context.preset.name.c_str());
    ImGui::Text("Frame: %llu", static_cast<unsigned long long>(frame.frameNumber));
    ImGui::Text("%.2f ms (%.1f FPS)", frame.deltaSeconds * 1000.0F,
                frame.deltaSeconds > 0.0F ? 1.0F / frame.deltaSeconds : 0.0F);
    ImGui::Separator();
    ImGui::Checkbox("Graphics pipeline", &context.preset.graphicsEnabled);
    ImGui::Checkbox("Compute pipeline", &context.preset.computeEnabled);
    ImGui::ColorEdit3("Clear color", &context.preset.clearColor.x);
    ImGui::SliderInt("Blur radius", &context.blur.radius, 1, 12);
    ImGui::BeginDisabled(!context.preset.computeEnabled);
    if (ImGui::Button("Start")) {
        context.blur.requested = true;
        context.blur.ready = false;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextUnformatted(context.blur.ready ? "Result ready" : "Waiting");
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(360.0F, 20.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430.0F, 640.0F), ImGuiCond_FirstUseEver);
    ImGui::Begin("Viewport");
    const ImVec2 available = ImGui::GetContentRegionAvail();
    if (available.x >= 64.0F && available.y >= 64.0F) {
        context.viewport.requestedWidth =
            static_cast<std::uint32_t>(std::floor(available.x));
        context.viewport.requestedHeight =
            static_cast<std::uint32_t>(std::floor(available.y));
        const ImTextureID texture =
            static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(viewportDescriptor_));
        ImGui::Image(texture, available);
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(810.0F, 20.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430.0F, 640.0F), ImGuiCond_FirstUseEver);
    ImGui::Begin("Blur Output");
    const ImVec2 blurAvailable = ImGui::GetContentRegionAvail();
    if (blurAvailable.x > 1.0F && blurAvailable.y > 1.0F &&
        context.blur.ready && blurDescriptor_ != VK_NULL_HANDLE &&
        context.blur.extent.width > 0 && context.blur.extent.height > 0) {
        const float scale = std::min(
            blurAvailable.x / static_cast<float>(context.blur.extent.width),
            blurAvailable.y / static_cast<float>(context.blur.extent.height));
        const ImVec2 imageSize{
            static_cast<float>(context.blur.extent.width) * scale,
            static_cast<float>(context.blur.extent.height) * scale};
        const ImTextureID texture =
            static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(blurDescriptor_));
        ImGui::Image(texture, imageSize);
    } else {
        ImGui::TextDisabled("Press Start to run the compute blur.");
    }
    ImGui::End();
    ImGui::Render();
}

void ImGuiModule::onRender(AppContext& context, const FrameInfo&) {
    constexpr VkClearColorValue background{{0.008F, 0.011F, 0.018F, 1.0F}};
    context.vulkan.beginColorPass(VK_ATTACHMENT_LOAD_OP_CLEAR, background);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), context.vulkan.commandBuffer());
    context.vulkan.endColorPass();
}

void ImGuiModule::onDetach(AppContext& context) {
    ImGui::SaveIniSettingsToDisk(iniPath_.c_str());
    if (viewportDescriptor_ != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(viewportDescriptor_);
        viewportDescriptor_ = VK_NULL_HANDLE;
    }
    if (blurDescriptor_ != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(blurDescriptor_);
        blurDescriptor_ = VK_NULL_HANDLE;
    }
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(context.vulkan.device(), descriptorPool_, nullptr);
    descriptorPool_ = VK_NULL_HANDLE;
}

} // namespace vkexp

