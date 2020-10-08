#include "pch.h"
#include "CommandBufferManager.h"

namespace scatter
{
void CommandBufferManager::init(VulkanDevice& vulkanDevice, VulkanRenderSequence& renderSequence, VkExtent2D& extent, VulkanVertexBuffer& vertexBuffer)	{
		createCommandPool(vulkanDevice);
		recordCommandBuffer(vulkanDevice.device, renderSequence, extent, vertexBuffer);
	}

void CommandBufferManager::destroy(VkDevice device)	{
		vkDestroyCommandPool(device, commandPool, nullptr);
	}

void CommandBufferManager::createCommandPool(VulkanDevice& vulkanDevice) {
	QueueFamilyIndices queueFamilyIndices = vulkanDevice.findQueueFamilies(vulkanDevice.physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0;

	if (vkCreateCommandPool(vulkanDevice.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool! \n");
	}
	else
	{
		std::cout << "successfully created command pool! \n";
	}
}

void CommandBufferManager::recordCommandBuffer(const VkDevice device, VulkanRenderSequence& renderSequence, VkExtent2D extent, VulkanVertexBuffer& vertexBuffer)
{

	commandBuffers.resize(renderSequence.framebuffers.size());
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers! \n");
	}
	else
	{
		std::cout << "successfully allocated command buffers! \n";
	}

	for (size_t i = 0; i < commandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record begin command buffer \n");
		}
		else
		{
			std::cout << "successfully recorder begin command buffer! \n";
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderSequence.renderPass;
		renderPassInfo.framebuffer = renderSequence.framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = extent;
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderSequence.pipeline);

		VkBuffer vertexBuffers[] = { vertexBuffer.buffer };
		VkDeviceSize offset[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offset);

		vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertexBuffer.bufferSize), 1, 0, 0);
		vkCmdEndRenderPass(commandBuffers[i]);
		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer! \n");
		}
		else
		{
			std::cout << "successfully recorded command buffer! \n";
		}
	}
}
}
