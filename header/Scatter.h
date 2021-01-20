#pragma once

#ifdef SCATTER_EXPORT
#define SCATTER_API __declspec(dllexport)
#else
#define SCATTER_API __declspec(dllimport)
#endif

/** @file This file contains doxygen lines */

namespace scatter {

/**
 * Describes the vertex position type
 */
enum class SCATTER_API VertexFormat : unsigned int {
    R32_SFLOAT = 100, /**< single 32 bit float */  
    R32G32_SFLOAT = 103, /**< two 32 bit floats */
    R32G32B32_SFLOAT = 106,  /**< three 32 bit floats */
    R32G32B32A32_SFLOAT = 109  /**< four 32 bit floats */
};


/**
 * Describes the index type
 */
enum class SCATTER_API IndexFormat : unsigned int {
    UINT16 = 0, /**< 16 bit unsigned integer */
    UINT32 = 1 /**< 32 bit unsigned integer */
};

/** @struct
 * Struct that describes the input layout. 
 */
struct SCATTER_API BufferDescription {
    /** vertexOffset describes the byte offset of the position type inside the vertex type. Defaults to zero. */
    uint32_t vertexOffset = 0; 
    /** vertexStride describes the total byte size of a single vertex element. Defaults to size of 3 floats. */
    uint32_t vertexStride = sizeof(float) * 3;
    /** vertexFormat describes the vertex position type. Defaults to 3 floats. */
    VertexFormat vertexFormat = VertexFormat::R32G32B32_SFLOAT;
    /** indexFormat describes the index type. Defaults to 32 bit unsigned integer */
    IndexFormat indexFormat = IndexFormat::UINT32;
};

/** @class
 * Object that contains the entire Scatter API. This object should only ever be constructed once in a host application.
 * It is implemented using the PIMPL idiom, hiding internal data from the resulting binary.
 */
class SCATTER_API Scatter {
public:
    /**
     * Default constructor. Only constructor.
     */
    Scatter();

    /**
     * Default destructor.
     */
    ~Scatter();

    /**
     * Compiler generated move constructor. Needed for PIMPL.
     */
    Scatter(Scatter&&) noexcept = default;

    /**
     * Default move assignment operator. Needed for PIMPL.
     */
    Scatter& operator=(Scatter&&) noexcept = default;

    /**
     * Init function that sets up the vulkan device and resources, takes no arguments.
     * @return void
     */
    void init();

    /**
     * Sets the vertex stride of the internal BufferDescription.
     * @return void
     */
    void setVertexStride(uint32_t stride);
    
    /**
     * Sets the vertex offset of the internal BufferDescription.
     * @return void
     */
    void setVertexOffset(uint32_t offset);
    
    /**
     * Sets the vertex format of the internal BufferDescription.
     * @return void
     */
    void setVertexFormat(VertexFormat format);
    
    /**
     * Sets the index format of the internal BufferDescription.
     * @return void
     */
    void setIndexFormat(IndexFormat format);

    /**
     * Sets the light direction. 
     * Takes 3 floats that make up a normal vector of where the light is looking at in world space.
     * @return void
     */
    void setLightDirection(float x, float y, float z);

    /**
     * Sets the inverse of the projection * view matrix. Works with column-major, not tested with others.
     * This performs a memcpy under the hood with a size of 3*4 floats.
     * Tested to work with glm::value_ptr(matrix).
     * Undefined behavior if anything weird is passed.
     * @return void
     */
    void setInverseViewProjectionMatrix(float* matrix);

    /**
     * Get the win32 HANDLE to the shadow texture's memory.
     * @return void* that refers to the shadow texture's memory. Can be type cast to win32 HANDLE.
     */
    void* getShadowTextureMemoryHandle();
    
    /**
     * Get the win32 HANDLE to the shadow texture's memory.
     * @return void* that refers to the shadow texture's memory. Can be type cast to win32 HANDLE.
     */
    void* getDepthTextureMemoryhandle();

    /**
     * Get the byte size of the shadow texture's memory.
     * Use this in conjuction with glImportMemoryWin32HandleEXT().
     * @return size_t of the shadow texture's memory in bytes.
     */
    size_t getShadowTextureMemorySize();

    /**
     * Get the byte size of the depth texture's memory.
     * Use this in conjuction with glImportMemoryWin32HandleEXT().
     * @return size_t of the depth texture's memory in bytes.
     */
    size_t getDepthTextureMemorySize();

    /**
     * Get the win32 HANDLE to the semaphore thats signaled before submit.
     * @return void* that refers to the exported semaphore. Can be type cast to win32 HANDLE.
     */
    void* getReadySemaphoreHandle();
    
    /**
     * Get the win32 HANDLE to the semaphore thats signaled when all submitted commands are done executing.
     * @return void* that refers to the exported semaphore. Can be type cast to win32 HANDLE.
     */
    void* getDoneSemaphoreHandle();

    /**
     * Submits the render commands to the graphics queue. 
     * The host application should synchronize around it using the exported semaphores.
     * @param width the width of shadow and depth texture, basically the renderable screen width.
     * @param height the height of shadow and depth texture, basically the renderable screen height.
     * @return void
     */
    void submit(uint32_t width, uint32_t height);

    /**
     * @param width width of the texture. Probably the screen region width you want to render to.
     * @param height height of the texture. Probably the screen region height you want to render to.
     * @return void
     */
    void createTextures(uint32_t width, uint32_t height);

    /**
     * Destroys the internal textures.
     * @return void
     */
    void destroyTextures();

    /**
     * @return uint64_t handle to the created bottom level acceleration structure. Keep it around for deletion or instancing.
     */
    [[nodiscard]] uint64_t addMesh(void* vertices, void* indices, unsigned int vertexCount, unsigned int indexCount);

    /**
     * Destroy a single mesh. Changes are visible after rebuilding the top level acceleration structure.
     * @param handle to the bottom level acceleration structure to delete.
     * @return void
     */
    void destroyMesh(uint64_t handle);

    /**
     * Add a single instance of a mesh.
     * @param handle to the bottom level acceleration structure to add.
     * @param transform world space transformation matrix of the mesh. See 'setInverseViewProjectionMatrix' for safety concerns.
     * @return void
     */
    void addInstance(uint64_t handle, float* transform);

    /**
     * Clears the top level acceleration structure. Changes are visible after rebuilding the top level acceleration structure.
     * @return void
     */
    void clearInstances();

    /**
     * After adding meshes and instances call this to finalize the build of the top level acceleration structure.
     * @return void
     */
    void build();

    /**
     * Explicit destroy function. Call this when you want Scatter's lifetime to end.
     * @return void
     */
    void destroy();

private:
    class Impl;
    Impl* pimpl;
};

}