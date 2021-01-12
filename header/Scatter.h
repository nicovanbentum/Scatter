#pragma once

#ifdef SCATTER_EXPORT
#define SCATTER_API __declspec(dllexport)
#else
#define SCATTER_API __declspec(dllimport)
#endif

namespace scatter {

enum class SCATTER_API VertexFormat : unsigned int {
    R32_SFLOAT = 100,
    R32G32_SFLOAT = 103,
    R32G32B32_SFLOAT = 106,
    R32G32B32A32_SFLOAT = 109
};

enum class SCATTER_API IndexFormat : unsigned int {
    UINT16 = 0,
    UINT32 = 1
};

struct SCATTER_API BufferDescription {
    uint32_t vertexOffset = 0;
    uint32_t vertexStride = sizeof(float) * 3;
    VertexFormat vertexFormat = VertexFormat::R32G32B32_SFLOAT;
    IndexFormat indexFormat = IndexFormat::UINT32;
};

class SCATTER_API Scatter {
public:
    Scatter();
    ~Scatter();
    Scatter(Scatter&&) noexcept = default;
    Scatter& operator=(Scatter&&) noexcept = default;

    // init API
    void init();

    // vertex input API
    void setVertexStride(uint32_t stride);
    void setVertexOffset(uint32_t offset);
    void setVertexFormat(VertexFormat format);
    void setIndexFormat(IndexFormat format);

    // shader constants API
    void setLightDirection(float x, float y, float z);
    void setInverseViewProjectionMatrix(float* matrix);

    // handle API
    void* getShadowTextureMemoryHandle();
    void* getDepthTextureMemoryhandle();
    
    size_t getShadowTextureMemorySize();
    size_t getDepthTextureMemorySize();

    void* getReadySemaphoreHandle();
    void* getDoneSemaphoreHandle();

    // submit API
    void submit(uint32_t width, uint32_t height);

    // texture API
    void createTextures(uint32_t width, uint32_t height);
    void destroyTextures();

    // as builder API
    uint64_t addMesh(void* vertices, void* indices, unsigned int vertexCount, unsigned int indexCount);
    void destroyMesh(uint64_t handle);
    void addInstance(uint64_t handle, float* transform);

    void clearInstances();
    void build();

    void destroy();

private:
    class Impl;
    Impl* pimpl;
};

}