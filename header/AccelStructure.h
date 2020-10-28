#pragma once

#include "Object.h"
#include "Extensions.h"

namespace scatter {

class AccelerationStructure {
public:
    void addGeometry(VkDevice device, const Object& obj, VkBuffer vertexBuffer, VkBuffer indexBuffer);

    void create(VkDevice device, VmaAllocator allocator, const RayTracingNV& rtNV);

    void destroy(VkDevice device, VmaAllocator allocator, const RayTracingNV& rtNV);

private:
    uint64_t handle;
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
    std::vector<VkGeometryNV> geometries;
    VkAccelerationStructureNV structure;
};

}
