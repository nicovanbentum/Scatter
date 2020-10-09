#pragma once

#include "ShaderManager.h"
#include "Swapchain.h"
#include "VulkanBuffer.h"

namespace scatter {

class VulkanRenderSequence {
    friend class VulkanApplication;
    friend class VulkanDevice;
public:
    void init(VkDevice device, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager);
    void destroyFramebuffers(VkDevice device);
    void destroy(VkDevice device);

    void createRenderPass(VkDevice device, const VulkanSwapchain& swapchain);
    void createGraphicsPipeline(VkDevice device, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager);
    void createFramebuffers(VkDevice device, const std::vector<VkImageView>& imageViews, VkExtent2D extent);

    size_t getFramebufferSize();
    VkRenderPass getRenderPass();
    VkFramebuffer getFramebuffer(size_t index);

private:
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkSemaphore semaphore;
    std::vector<VkFramebuffer> framebuffers;
    VkRenderPass renderPass;

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
};
}