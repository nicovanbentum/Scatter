#include "pch.h"
#include "VertexBuffer.h"

namespace scatter {

void VulkanBuffer::init(VulkanDevice& device, const void* vectorData, size_t sizeInBytes, VkBufferUsageFlagBits usage) {
  
    auto [stagingBuffer, stagingAlloc, stagingAllocInfo] = device.createStagingBuffer(sizeInBytes);

    memcpy(stagingAllocInfo.pMappedData, vectorData, sizeInBytes);

    VkBufferCreateInfo vertexBufferInfo{};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = sizeInBytes;
    vertexBufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vmaCreateBuffer(device.allocator, &vertexBufferInfo, &allocCreateInfo, &buffer, &alloc, &allocInfo);

    auto commandBuffer = device.beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = sizeInBytes;
    vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer, 1, &copyRegion);

    device.endSingleTimeCommands(commandBuffer);

    vmaDestroyBuffer(device.allocator, stagingBuffer, stagingAlloc);
}

void VulkanBuffer::destroy(const VulkanDevice& device) {
    vmaDestroyBuffer(device.allocator, buffer, alloc);
}

}