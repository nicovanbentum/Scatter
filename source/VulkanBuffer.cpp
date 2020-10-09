#include "pch.h"
#include "VulkanBuffer.h"

namespace scatter {

void VulkanBuffer::init(VulkanDevice& device, const void* vectorData, size_t sizeOfData, VkBufferUsageFlagBits usageFlag) {
    VkBuffer stagingBuffer;
    VmaAllocation stagingAlloc;
    VmaAllocationInfo stagingAllocInfo;

    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = sizeOfData;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo stagingAllocCreateInfo = {};
    stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    vmaCreateBuffer(device.allocator, &stagingBufferInfo, &stagingAllocCreateInfo, &stagingBuffer, &stagingAlloc, &stagingAllocInfo);

    memcpy(stagingAllocInfo.pMappedData, vectorData, stagingBufferInfo.size);

    VkBufferCreateInfo vertexBufferInfo{};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = sizeOfData;
    vertexBufferInfo.usage = usageFlag | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vmaCreateBuffer(device.allocator, &vertexBufferInfo, &allocCreateInfo, &buffer, &alloc, &allocInfo);

    auto commandBuffer = VulkanApplication::beginSingleTimeCommands(device);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = sizeOfData;
    vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer, 1, &copyRegion);

    VulkanApplication::endSingleTimeCommands(device, commandBuffer);

    vmaDestroyBuffer(device.allocator, stagingBuffer, stagingAlloc);
}

void VulkanBuffer::destroy(const VulkanDevice& device) {
    vmaDestroyBuffer(device.allocator, buffer, alloc);
}

}