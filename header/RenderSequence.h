#pragma once

#include "ShaderManager.h"
#include "Swapchain.h"
#include "VertexBuffer.h"

namespace scatter {

class VulkanRenderSequence {
public:
    void init(VkDevice device, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager);
    void destroyFramebuffers(VkDevice device);
    void destroy(VkDevice device);

    void createRenderPass(VkDevice device, const VulkanSwapchain& swapchain);
    void createGraphicsPipeline(VkDevice device, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager);
    void createFramebuffers(VkDevice device, const std::vector<VkImageView>& imageViews, VkExtent2D extent);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, VkExtent2D extent, VkBuffer vertexBuffer, uint32_t vertexCount, uint8_t framebufferIndex);

    size_t getFramebuffersCount() { return framebuffers.size(); }

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