#pragma once

#include "AccelStructure.h"
#include "VulkanBuffer.h"

namespace scatter {

enum class VertexFormat : unsigned int {
    R32_SFLOAT = 100,
    R32G32_SFLOAT = 103,
    R32G32B32_SFLOAT = 106,
    R32G32B32A32_SFLOAT = 109
};

enum class IndexFormat : unsigned int {
    UINT16 = 0,
    UINT32 = 1
};

struct BufferDescription {
    uint32_t vertexOffset = 0;
    uint32_t vertexStride = sizeof(float) * 3;
    VertexFormat vertexFormat = VertexFormat::R32G32B32_SFLOAT;
    IndexFormat indexFormat = IndexFormat::UINT32;
};

/*
     auto scatter = scatter::createContext();
     scatter.setVertexStride(sizeof(Vertex));
     scatter.setVertexOffset(offsetof(Vertex, pos));
     scatter.setVertexFormat(VertexFormat::R32G32B32A32_SFLOAT);
     scatter.setIndexFormat(IndexFormat::UINT32);
*/


/*
     auto builder = scatter.getBuilder();
     auto mesh = builder.AddMesh(vertices.data(), vertices.size(), 
                                    indices.data(), indices.size());

     builder.addInstance(mesh, transform1);
     builder.addInstance(mesh, transform2);
     builder.rebuild();
 */



/*
    scatter.createResources(1920, 1080);

     HANDLE depthTextureHandle = scatter.getDepthMemoryHandle();
     size_t depthTextureMemSize = scatter.getDepthMemorySize();

     HANDLE shadowTextureHandle = scatter.getShadowMemoryHandle();
     size_t shadowTextureMemSize = scatter.getShadowMemorySize();

     HANDLE readySemaphore = scatter.getReadySemaphore();
     HANDLE doneSemaphore = scatter.getDoneSemaphore();
*/

/*
    glSignalSemaphoreEXT(readySemaphore);

    scatter.submit();

    glWaitSemaphoreEXT(doneSemaphore);
*/



class AccelerationStructureBuilder {
public:

    AccelerationStructureBuilder(const BufferDescription& bufferDescription) 
        : attribDesc(bufferDescription) {

    }

    void addInstance(uint64_t handle, float* transform) {
        assert(transform);
        
        VkAccelerationStructureInstanceNV instance;
        instance.instanceCustomIndex = 0;
        instance.flags = 0xff;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
        instance.accelerationStructureReference = handle;

        std::memcpy(&instance.transform, transform, sizeof(instance.transform));

        instances.push_back(std::move(instance));
    }

    uint64_t addMesh(float* vertices, float* indices, unsigned int vertexCount, unsigned int indexCount, float* transform = nullptr) {
        auto& mesh = meshes.emplace_back(vertices, indices, vertexCount, indexCount);

        if (transform) {
            std::memcpy(glm::value_ptr(mesh.transform), transform, sizeof(float) * 16);
        } else {
            mesh.transform = glm::mat4(1.0f);
        }

        VulkanBuffer vertexBuffer;
        VulkanBuffer indexBuffer;
        //vertexBuffer.init(device, vertices, attribDesc.vertexStride * vertexCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        
        uint32_t indexSize = 0;
        switch (attribDesc.indexFormat) {
            case IndexFormat::UINT32: indexSize = sizeof(uint32_t); break;
            case IndexFormat::UINT16: indexSize = sizeof(uint16_t); break;
        }

        //indexBuffer.init(device, indices, indexSize * indexCount, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);


        VkGeometryNV geometry{};
        VkGeometryTrianglesNV& triangle = geometry.geometry.triangles;

        geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
        triangle.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
        triangle.vertexData = vertexBuffer.getBuffer();
        triangle.vertexOffset = 0;
        triangle.vertexCount = mesh.vertexCount;
        triangle.vertexStride = sizeof(Vertex);
        triangle.vertexFormat = static_cast<VkFormat>(attribDesc.vertexFormat);
        triangle.indexData = indexBuffer.getBuffer();
        triangle.indexOffset = 0;
        triangle.indexCount = mesh.indexCount;
        triangle.indexType = static_cast<VkIndexType>(attribDesc.indexFormat);
        triangle.transformData = VK_NULL_HANDLE;
        triangle.transformOffset = 0;
        geometry.geometry.aabbs = {};
        geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

        // create bottom level acceleration structure 
        VkAccelerationStructureCreateInfoNV BLAScreateInfo{};
        BLAScreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
        BLAScreateInfo.info.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
        BLAScreateInfo.info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
        BLAScreateInfo.info.instanceCount = 0;
        BLAScreateInfo.info.geometryCount = 1;
        BLAScreateInfo.info.pGeometries = &geometry;

        //bottomLevels.emplace_back().init(device.device, device.allocator, &BLAScreateInfo);
        //bottomLevels.back().record(device, &BLAScreateInfo);

        return bottomLevels.back().handle;
    }

    AccelerationStructureBuilder* build() {

        // create top level acceleration structure
        VkAccelerationStructureCreateInfoNV TLAScreateInfo{};
        TLAScreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
        TLAScreateInfo.info.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
        TLAScreateInfo.info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
        TLAScreateInfo.info.instanceCount = 1;
    }

private:
    struct Mesh {
        float* vertices; 
        float* indices; 
        unsigned int vertexCount; 
        unsigned int indexCount;
        glm::mat4 transform;
    };

    TopLevelAS TLAS;
    BottomLevelAS BLAS;
    BufferDescription attribDesc;

    std::vector<BottomLevelAS> bottomLevels;
    std::vector<VkAccelerationStructureInstanceNV> instances;

    // temporary
    std::vector<Mesh> meshes;
    std::vector<Instance> instances;
};

}