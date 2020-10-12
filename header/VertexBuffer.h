#pragma once
#include "pch.h"
#include "Device.h"

namespace scatter {

class VulkanBuffer {
public:
    void init(VulkanDevice& device, const void* vectorData, size_t sizeInBytes, VkBufferUsageFlagBits usage);
    void destroy(const VulkanDevice& device);

    VkBuffer getBuffer() { return buffer; }

private:
    VkBuffer buffer;
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
};
}
