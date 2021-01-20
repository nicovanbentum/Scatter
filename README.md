# Scatter
Ray traced hard shadows for OpenGL using Vulkan RTX and Windows memory handles.

## How does it work?
Scatter is a small Vulkan library that produces a screen space shadow texture based on a single directional light. As for now, the technique is 1spp hard shadows.
It requires a rendered depth buffer as it reconstructs world positions using depth and view-projection matrices. It ray traces from world position towards the light and checks for blocking geometry. For this reason Vulkan needs to know about your scene's geometry and build an acceleration structure out of it. Scatter provides a few simple functions to set this up. 

## The API

### Setup
This one is pretty simple, create an instance of the ```scatter::Scatter``` class and call its ```init()``` member function. The entire API is implemented as member functions of this object. It is implemented using the PIMPL principle so the build only exports necessary functions. We are aware that this is an inconvenience for debugging.

### Vertex Input
Scatter only supports submission of triangle meshes so you'll need to tell Scatter what that data looks like. The API assumes you have separate buffers for vertices and indices and that they're accessible from host memory, for more details see the Acceleration Structure part of this readme.

let's say your vertex layout is a simple struct:
``` 
struct Vertex {
  vec2 texcoord;
  vec3 position;
  vec3 normal;
} 
```

To tell Scatter how it should extract the positional data from this you call:

```
scatter.setVertexOffset(offsetof(Vertex, position));
scatter.setVertexStride(sizeof(Vertex));
scatter.setIndexFormat(IndexFormat::UINT32);
scatter.setVertexFormat(VertexFormat::R32G32B32_SFLOAT);
```

### Textures
To get communication working between Vulkan and OpenGL we need to create all our textures in Vulkan and retrieve their memory in OpenGL.
Scatter creates these textures internally when you call ```createTextures(1920, 1080)```. When the application ends or you want to resize based on window dimensions you can call ```destroyTextures()``` to invalidate the textures **and** handles.

To get the handles for use in OpenGL you call:
```
auto depthHandle = scatter.getDepthTextureMemoryhandle();
const auto depthSize = scatter.getDepthTextureMemorySize();

auto shadowHandle = scatter.getShadowTextureMemoryHandle();
const auto shadowSize = scatter.getShadowTextureMemorySize();
```

You will need to have the ```gl_ext_memory_object_win32``` extension to create the textures, example:

```
glCreateMemoryObjectsEXT(1, &depthTextureMemory);
glImportMemoryWin32HandleEXT(depthTextureMemory, depthSize, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, depthHandle);
```

and assign it to your texture using

```
glTextureStorageMem2DEXT(depthTexture, 1, GL_DEPTH_COMPONENT32F, width, height, depthTextureMemory, 0);
```

_Note:_ the formats should be GL_DEPTH_COMPONENT32F for depth and GL_RGBA8 for the shadow texture. They correspond to the format of the Scatter's vulkan texture.
We are aware that this isn't ideal, the user should be able to specify formats in the future.

### Acceleration Structure
Ray tracing extensions build an internal bounding hierarchy volume out of the triangles you give it. To keep interop dependencies to a minimum and maintain a clean API its implemented as 3 functions. First step is to add meshes:
```
for(auto mesh : scene) {
  uin64_t handle = scatter.addMesh(mesh.vertices.data(), mesh.vertices.size(), mesh.indices.data(), mesh.indices.size());
}
```

You'll need the returned handle to remove/update meshes and creating the final structure.
Next is instancing and building. You can instantiate meshes using a single transformation matrix.
This part has negligible performance impact so do it every frame!
**__note__**: Vulkan expects row-major matrices so make sure to transpose if you're using a library like GLM.
Don't worry about the pointer argument, the API performs a `memcpy` for 16 floats so you can use local variables.

```
scatter.clearInstances();

for(auto mesh : scene) {
  scatter.addInstance(mesh.storedHandle, &mesh.transform);
}

scatter.build();
```

To remove meshes call `destroyMesh(handle)`, possibly re-adding them for e.g animated vertices.

## Build

- Install and enable [VCPKG](https://github.com/microsoft/vcpkg) system-wide integration and install glfw3 and GLM.
- ```git clone``` this repository or use a Git client.
- Make sure you have the latest submodules using ``` git submodule update --recursive --init```.
- Make sure you have the Vulkan SDK installed.
- Download and copy over the debug libraries into your SDK version ([link](https://files.lunarg.com/)).
- Build the Visual Studio solution.
