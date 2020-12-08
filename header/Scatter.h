#pragma once

#include "RenderSequence.h"
#include "AccelStructure.h"

namespace scatter {

struct Mesh {
    Mesh(void* vertices, void* indices, unsigned int vertexCount, unsigned int indexCount) :
    vertices(vertices), indices(indices), vertexCount(vertexCount), indexCount(indexCount) {}
    void* vertices; 
    void* indices; 
    unsigned int vertexCount; 
    unsigned int indexCount;
};

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

class Scatter {
public:
    // init API
    void init(uint32_t width, uint32_t height);

    // vertex input API
    void setVertexStride(uint32_t stride) { attribDesc.vertexStride = stride; }
    void setVertexOffset(uint32_t offset) { attribDesc.vertexOffset = offset; }
    void setVertexFormat(VertexFormat format) { attribDesc.vertexFormat = format; };
    void setIndexFormat(IndexFormat format) { attribDesc.indexFormat = format; }

    // shader constants API
    void setLightDirection(float x, float y, float z);
    void setInverseViewProjectionMatrix(float* matrix);

    // handle API
    HANDLE getShadowTextureMemoryHandle();
    HANDLE getDepthTextureMemoryhandle();
    
    size_t getShadowTextureMemorySize();
    size_t getDepthTextureMemorySize();

    HANDLE getReadySemaphoreHandle();
    HANDLE getDoneSemaphoreHandle();

    // submit API
    void submit(uint32_t width, uint32_t height);

    // texture API
    void createTextures(uint32_t width, uint32_t height);
    void destroyTextures();

    // as builder API
    uint64_t addMesh(void* vertices, void* indices, unsigned int vertexCount, unsigned int indexCount);
    void addInstance(uint64_t handle, float* transform);
    void build();

    void clear();

    ~Scatter();

private:
    VkFence cpuFence;
    VulkanDevice device;
    RayTracedShadowsSequence rtx;
    VulkanShaderManager shaderManager;
    VkSemaphore readySemaphore, doneSemaphore;

    // geometry stuff
    BufferDescription attribDesc;
    std::vector<Mesh> meshes;
    std::vector<BottomLevelAS> bottomLevels;
    TopLevelAS TLAS;
    std::vector< VkAccelerationStructureInstanceNV> instances;
};

}