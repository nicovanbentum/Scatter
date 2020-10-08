#pragma once

#include "ShaderManager.h"
#include "Swapchain.h"
#include "VertexBuffer.h"

namespace scatter {

    class VulkanRenderSequence {
		friend class CommandBufferManager;
    public:
        void init(VkDevice device, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager);
        void destroy(VkDevice device);

        void createRenderPass(VkDevice device, const VulkanSwapchain& swapchain);
		void createGraphicsPipeline(VkDevice device, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager);
		void createFramebuffers(VkDevice device, const std::vector<VkImageView>& imageViews, VkExtent2D extent);


    private:
        VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;

        VkSemaphore semaphore;
		std::vector<VkFramebuffer> framebuffers;
        VkRenderPass renderPass;
    };
}