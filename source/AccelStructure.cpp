#include "pch.h"
#include "AccelStructure.h"
#include "VulkanBuffer.h"

namespace scatter {

void BottomLevelAS::init(VkDevice device, VmaAllocator allocator, VkAccelerationStructureCreateInfoNV* createInfo) {
    if (vk_nv_ray_tracing::vkCreateAccelerationStructureNV(device, createInfo, nullptr, &as) != VK_SUCCESS) {
        throw std::runtime_error("failed vkCreateAccelerationStructureNV");
    } else {
        std::cout << "created acceleration structure!\n";
    }

    // get acceleration structure memory requirements
    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
    memoryRequirementsInfo.accelerationStructure = as;

    VkMemoryRequirements2 memoryRequirements;
    vk_nv_ray_tracing::vkGetAccelerationStructureMemoryRequirementsNV(device, &memoryRequirementsInfo, &memoryRequirements);

    // allocate the AS memory
    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.memoryTypeBits = memoryRequirements.memoryRequirements.memoryTypeBits;

    if (vmaAllocateMemory(allocator, &memoryRequirements.memoryRequirements, &allocCreateInfo, &alloc, &allocInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate acceleration structure memory");
    }

    // bind the AS memory to the AS
    VkBindAccelerationStructureMemoryInfoNV memoryInfo{};
    memoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    memoryInfo.accelerationStructure = as;
    memoryInfo.memory = allocInfo.deviceMemory;
    memoryInfo.memoryOffset = allocInfo.offset;

    if (vk_nv_ray_tracing::vkBindAccelerationStructureMemoryNV(device, 1, &memoryInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to bind acceleration structure memory");
    }

    // set the handle
    if (vk_nv_ray_tracing::vkGetAccelerationStructureHandleNV(device, as, sizeof(uint64_t), &handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to get acceleration structure handle");
    }
}

void BottomLevelAS::record(VulkanDevice& device, VkAccelerationStructureCreateInfoNV* createInfo) {
    // get the memory requirements for the scratch buffer
    VkAccelerationStructureMemoryRequirementsInfoNV scratchRequirementsInfo{};
    scratchRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    scratchRequirementsInfo.type = VkAccelerationStructureMemoryRequirementsTypeNV::VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
    scratchRequirementsInfo.accelerationStructure = as;

    VkMemoryRequirements2 scratchRequirements;
    vk_nv_ray_tracing::vkGetAccelerationStructureMemoryRequirementsNV(device.device, &scratchRequirementsInfo, &scratchRequirements);

    // create the scratch buffer
    VkBufferCreateInfo scratchBufferInfo{};
    scratchBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    scratchBufferInfo.size = scratchRequirements.memoryRequirements.size;
    scratchBufferInfo.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
    scratchBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkBuffer scratchBuffer;
    VmaAllocation scratchBufferAlloc;

    vmaCreateBuffer(device.allocator, &scratchBufferInfo, &allocCreateInfo, &scratchBuffer, &scratchBufferAlloc, nullptr);

    // record the command buffer
    auto cmdBuffer = device.beginSingleTimeCommands();

    vk_nv_ray_tracing::vkCmdBuildAccelerationStructureNV(cmdBuffer, &createInfo->info, VK_NULL_HANDLE, 0, VK_FALSE, as, VK_NULL_HANDLE, scratchBuffer, 0);

    device.endSingleTimeCommands(cmdBuffer);

    // destroy scratch buffer
    vmaDestroyBuffer(device.allocator, scratchBuffer, scratchBufferAlloc);
}

void BottomLevelAS::destroy(VkDevice device, VmaAllocator allocator) {
    vk_nv_ray_tracing::vkDestroyAccelerationStructureNV(device, as, nullptr);
    vmaFreeMemory(allocator, alloc);
}

} // scatter