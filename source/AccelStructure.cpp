#include "pch.h"
#include "AccelStructure.h"

namespace scatter {

void AccelerationStructure::addGeometry(VkDevice device, const Object& obj, VkBuffer vertexBuffer, VkBuffer indexBuffer) {
    VkGeometryNV geometry{};
    VkGeometryTrianglesNV& triangle = geometry.geometry.triangles;

    geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
    triangle.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
    triangle.vertexData = vertexBuffer;
    triangle.vertexOffset = obj.vertexOffset;
    triangle.vertexCount = obj.vertices.size();
    triangle.vertexStride = sizeof(Vertex);
    triangle.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    triangle.indexData = indexBuffer;
    triangle.indexOffset = 0;
    triangle.indexCount = obj.indices.size();
    triangle.indexType = VK_INDEX_TYPE_UINT16;
    triangle.transformData = VK_NULL_HANDLE;
    triangle.transformOffset = 0;
    geometry.geometry.aabbs = {};
    geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

    geometries.push_back(geometry);
}

void AccelerationStructure::create(VkDevice device, VmaAllocator allocator, const RayTracingNV& rtNV) {
    VkAccelerationStructureInfoNV info{};
    info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    info.instanceCount = 0;
    info.geometryCount = geometries.size();
    info.pGeometries = geometries.data();

    VkAccelerationStructureCreateInfoNV createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    createInfo.info = info;

    if (rtNV.vkCreateAccelerationStructureNV(device, &createInfo, nullptr, &structure) != VK_SUCCESS) {
        throw std::runtime_error("failed vkCreateAccelerationStructureNV");
    } else {
        std::cout << "created acceleration structure!\n";
    }

    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
    memoryRequirementsInfo.accelerationStructure = structure;

    VkMemoryRequirements2 memoryRequirements;
    rtNV.vkGetAccelerationStructureMemoryRequirementsNV(device, &memoryRequirementsInfo, &memoryRequirements);

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.memoryTypeBits = memoryRequirements.memoryRequirements.memoryTypeBits;

    vmaAllocateMemory(allocator, &memoryRequirements.memoryRequirements, &allocCreateInfo, &alloc, &allocInfo);
    VkBindAccelerationStructureMemoryInfoNV memoryInfo{};
    memoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    memoryInfo.accelerationStructure = structure;
    memoryInfo.memory = allocInfo.deviceMemory;
    memoryInfo.memoryOffset = allocInfo.offset;

    if (rtNV.vkBindAccelerationStructureMemoryNV(device, 1, &memoryInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to bind acceleration structure memory");
    }

    if (rtNV.vkGetAccelerationStructureHandleNV(device, structure, sizeof(uint64_t), &handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to get acceleration structure handle");
    }
}

void AccelerationStructure::destroy(VkDevice device, VmaAllocator allocator, const RayTracingNV& rtNV) {
    rtNV.vkDestroyAccelerationStructureNV(device, structure, nullptr);
    vmaFreeMemory(allocator, alloc);
}

}