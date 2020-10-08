#pragma once

#include "ShaderManager.h"
#include "Swapchain.h"

namespace scatter {
	struct Vertex
	{
		glm::vec2 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription getBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 2> attributeDescription{};
			attributeDescription[0].binding = 0;
			attributeDescription[0].location = 0;
			attributeDescription[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescription[0].offset = offsetof(Vertex, pos);

			attributeDescription[1].binding = 0;
			attributeDescription[1].location = 1;
			attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription[1].offset = offsetof(Vertex, color);
			return attributeDescription;
		}
	};


    class VulkanRenderSequence {
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