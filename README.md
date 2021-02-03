# Scatter
Ray traced hard shadows for OpenGL using Vulkan RTX and Windows memory handles.

## Requirements
+ C++ 17
+ GPU support for:
    - gl_ext_memory_object_win32
    - vk_nv_ray_tracing
    
## How does it work?
Scatter is a small Vulkan library that produces a screen space shadow texture based on a single directional light. As for now, the technique is 1spp hard shadows.
It requires a rendered depth buffer as it reconstructs world positions using depth and view-projection matrices. It ray traces from world position towards the light and checks for blocking geometry. For this reason Vulkan needs to know about your scene's geometry and build an acceleration structure out of it. Scatter provides a few simple functions to set this up. It provides an interface for getting Win32 handles to the result, which can be imported into OpenGL at runtime.

## The API

### Setup
This one is pretty simple, create an instance of the ```scatter::Scatter``` class and call its ```init()``` member function. The entire API is implemented as member functions of this object. It is implemented using the PIMPL principle so the build only exports necessary functions. We are aware that this is inconvenient for debugging.

### Vertex Input
Scatter works on triangle meshes so you'll need to tell Scatter what that data looks like.
let's say your vertex layout is a simple struct:

    struct Vertex {
      float2 texcoord;
      float3 position;
      float3 normal;
    } 


To tell Scatter how it should extract the positional data from this you call:


    scatter.setVertexOffset(offsetof(Vertex, position));
    scatter.setVertexStride(sizeof(Vertex));
    scatter.setVertexFormat(VertexFormat::R32G32B32_SFLOAT);


Indices are either 16 or 32 bit unsigned integers:

    scatter.setIndexFormat(IndexFormat::UINT32); 


### Textures
To interop between Vulkan and OpenGL we need to create all textures are created in Vulkan and exported to OpenGL.
Scatter creates these textures internally when you call ```createTextures(width, height)```. 
When the application ends or you want to resize based on window dimensions you can call ```destroyTextures()``` to invalidate the textures **and** handles.
Importing to OpenGL requires the handles:

    auto depthHandle = scatter.getDepthTextureMemoryhandle();
    const auto depthSize = scatter.getDepthTextureMemorySize();

    auto shadowHandle = scatter.getShadowTextureMemoryHandle();
    const auto shadowSize = scatter.getShadowTextureMemorySize();


You will need support for the ```gl_ext_memory_object_win32``` extension to create the textures, example:

    GLuint depthTextureMemory;
    glCreateMemoryObjectsEXT(1, &depthTextureMemory);
    glImportMemoryWin32HandleEXT(depthTextureMemory, depthSize, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, depthHandle);

and assign it to your texture using

    unsigned int depthTexture;
    glCreateTextures(GL_TEXTURE_2D, 1, &depthTexture);
    glTextureStorageMem2DEXT(depthTexture, 1, GL_DEPTH_COMPONENT32F, width, height, depthTextureMemory, 0);

_Note:_ the formats should be `GL_DEPTH_COMPONENT32F` for depth and `GL_RGBA8` for the shadow texture. They correspond to the format of the Vulkan textures.
We are aware that this isn't ideal, the user should be able to specify formats in the future.

### Acceleration Structure
Ray tracing extensions build an internal bounding hierarchy volume out of the triangles you give it. To keep interop dependencies to a minimum and maintain a clean API its implemented as 5 functions. First step is to add meshes:

    for(auto mesh : scene) {
      uin64_t handle = scatter.addMesh(mesh.vertices.data(), mesh.vertices.size(), mesh.indices.data(), mesh.indices.size());
    }


You'll need the returned handle to remove/update meshes and to create the final structure.
You can instantiate meshes using a single transformation matrix.
Instantiation has negligible performance impact so do it every frame!
**__note__**: Vulkan expects row-major matrices so make sure to transpose if you're using a library like GLM.
Don't worry about the pointer argument, the API performs a `memcpy` for 16 floats so you can use local variables.

    scatter.clearInstances();

    for(auto mesh : scene) {
      scatter.addInstance(mesh.storedHandle, &mesh.transform);
    }

    scatter.build();

To remove meshes call `destroyMesh(handle)`, possibly re-adding them for e.g animated vertices.

Do note that this is a naive implementation that creates Vulkan buffers on-the-fly and only keeps the final acceleration structure around.
It also doesn't perform acceleration structure updates, only full rebuilds.

### Synchronization
GPUs are highly parallel and OpenGL does whatever it wants, whenever it wants. You'll need to create two OpenGL semaphores to tell the GPU when Scatter can start and signal back when it is done. Much like textures, Vulkan creates and exports the objects:

    auto readyHandle = scat.getReadySemaphoreHandle();
    auto doneHandle = scat.getDoneSemaphoreHandle();

Importing is also similar:

    unsigned int semaphore;
    glGenSemaphoresEXT(1, &semaphore);
    glImportSemaphoreWin32HandleEXT(semaphore, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, readyHandle);
    
### Submission
Scatter needs camera matrices and a light direction at submission time. The camera matrix should be the inverse of projection * view:
    
    auto invViewProj = inverse(projection * view);
    scatter.setInverseViewProjectionMatrix(&invViewProj);
    
To set the light direction:
    
    scatter.setLightDirection(light.direction.x, light.direction.y, light.direction.z);
    
Vulkan requires explicit image layout transfers so we need to do this in OpenGL as well. 
Since we don't really care about the initial layout we set it to general.
Scatter's submission waits for the exported ready semaphore to be signaled, so we do that now:
    
    GLuint textures[2] = { shadowTexture, depthTexture };
    GLenum vkLayout[2] = { GL_LAYOUT_GENERAL_EXT, GL_LAYOUT_GENERAL_EXT };
    glSignalSemaphoreEXT(readySemaphore, 0, nullptr, 2, textures, vkLayout);
    
And finally, to execute the ray tracing commands call

    scatter.submit(width, height);
    
Before the host application can consume the shadow texture it has to assure Scatter's work is done by signaling the done semaphore.

    GLenum glLayout[2] = { GL_LAYOUT_SHADER_READ_ONLY_EXT, GL_LAYOUT_DEPTH_STENCIL_ATTACHMENT_EXT };
    glWaitSemaphoreEXT(completeSemaphore, 0, nullptr, 2, &depthTexture, glLayout);
    
This also transitions both textures to their final layout.
From here you can bind the shadow image to a shader and apply it to lighting calculations.

## Build

- Make sure you have the Vulkan SDK installed
- Download and copy over the debug libraries into your SDK version ([link](https://files.lunarg.com/))
- Install [vcpkg](https://github.com/microsoft/vcpkg) and enable user wide integration.

- ```git clone``` this repository or use a Git client
- Make sure you have the latest submodules using ``` git submodule update --recursive --init```
- Build the Visual Studio solution

## Linking

- Static link against Scatter.lib
- Copy Scatter.dll to the executable directory
- `#include "Scatter.h"`

## Documentation

The docs are included with the repository, but only the `Scatter.h` header file is documented using Doxygen. 
If someone knows an easy way to generate doxygen for a single file, let us know.
