#pragma once

#include "Object.h"
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

struct Instance {
    glm::mat4 transform;
    uint32_t instanceID;
    uint32_t hitGroupIndex;
    VkAccelerationStructureNV BLAS;
};

struct TopLevelAS {
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
    VkAccelerationStructureNV as;

    void init(VkDevice device, VmaAllocator allocator, VkAccelerationStructureCreateInfoNV* createInfo);
    void record(VulkanDevice& device, Instance* instances, VkAccelerationStructureCreateInfoNV* createInfo);
    void destroy(VkDevice device, VmaAllocator allocator);
};


}
