#pragma once

#include "Extensions.h"
#include "Device.h"

namespace scatter {

struct BottomLevelAS {
    uint64_t handle;
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
    VkAccelerationStructureNV as;

    void init(VkDevice device, VmaAllocator allocator, VkAccelerationStructureCreateInfoNV* createInfo);
    void record(VulkanDevice& device, VkAccelerationStructureCreateInfoNV* createInfo);
    void destroy(VkDevice device, VmaAllocator allocator);
};

struct TopLevelAS {
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
    VkAccelerationStructureNV as = nullptr;

    void init(VkDevice device, VmaAllocator allocator, VkAccelerationStructureCreateInfoNV* createInfo);
    void record(VulkanDevice& device, VkAccelerationStructureInstanceNV* instances, VkAccelerationStructureCreateInfoNV* createInfo);
    void destroy(VkDevice device, VmaAllocator allocator);
};




}
