#include "pch.h"
#include "VertexBuffer.h"

namespace scatter
{

void VulkanVertexBuffer::init(VulkanDevice& device, CommandBufferManager& commandBufferManager, const std::vector<Vertex>& vertices)
{
	VkBuffer stagingBuffer;
	VmaAllocation stagingAlloc;
	VmaAllocationInfo stagingAllocInfo;

	bufferSize = vertices.size();
	VkBufferCreateInfo stagingBufferInfo{};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.size = sizeof(Vertex) * vertices.size();
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo stagingAllocCreateInfo = {};
	stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	stagingAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	vmaCreateBuffer(device.allocator, &stagingBufferInfo, &stagingAllocCreateInfo, &stagingBuffer, &stagingAlloc, &stagingAllocInfo);

	memcpy(stagingAllocInfo.pMappedData, vertices.data(), stagingBufferInfo.size);

	VkBufferCreateInfo vertexBufferInfo{};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.size = sizeof(vertices[0]) * vertices.size();
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocCreateInfo{};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vmaCreateBuffer(device.allocator, &vertexBufferInfo, &allocCreateInfo, &buffer, &alloc, &allocInfo);

	auto commandBuffer = commandBufferManager.beginSingleTimeCommands(device);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = vertices.size() * sizeof(Vertex);
	vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer, 1, &copyRegion);

	commandBufferManager.endSingleTimeCommands(device, commandBuffer);

	vmaDestroyBuffer(device.allocator, stagingBuffer, stagingAlloc);
}

void VulkanVertexBuffer::destroy(const VulkanDevice& device)
{
	vmaDestroyBuffer(device.allocator, buffer, alloc);
}

uint32_t VulkanVertexBuffer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VulkanDevice& device)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type! \n");
}
}