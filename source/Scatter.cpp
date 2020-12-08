#include "pch.h"
#include "Scatter.h"
#include "..\header\Scatter.h"

namespace scatter {

void Scatter::setLightDirection(float x, float y, float z) {
    rtx.pushData.lightDirection = glm::vec4(x, y, z, 1.0);
}

void Scatter::setInverseViewProjectionMatrix(float* matrix) {
    memcpy(glm::value_ptr(rtx.pushData.inverseViewProjection), matrix, sizeof(glm::mat4));
}

void Scatter::init(uint32_t width, uint32_t height) {
    device.init();
    shaderManager.init(device.device);
    rtx.init(device.device, device.allocator, device.physicalDevice, shaderManager, { width, height });
    rtx.createDescriptorSets(device.device, device.descriptorPool);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkExportSemaphoreCreateInfo exportInfo = {};
    exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
    exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    semaphoreInfo.pNext = &exportInfo;

    vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &doneSemaphore);
    vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &readySemaphore);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(device.device, &fenceInfo, nullptr, &cpuFence);
}

HANDLE Scatter::getShadowTextureMemoryHandle() {
    return rtx.getShadowTextureMemoryHandle(device.device);
}

size_t Scatter::getShadowTextureMemorySize() {
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device.device, rtx.shadowsTexture.image, &memRequirements);
    return memRequirements.size;
}

size_t Scatter::getDepthTextureMemorySize() {
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device.device, rtx.depthTexture.image, &memRequirements);
    return memRequirements.size;
}

HANDLE Scatter::getReadySemaphoreHandle() {
    auto vkGetSemaphoreWin32HandleKHR = PFN_vkGetSemaphoreWin32HandleKHR(vkGetDeviceProcAddr(device.device, "vkGetSemaphoreWin32HandleKHR"));
    
    HANDLE handle;
    
    VkSemaphoreGetWin32HandleInfoKHR info = {};
    info.sType          = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
    info.semaphore      = readySemaphore;
    info.handleType     = VkExternalSemaphoreHandleTypeFlagBits::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    
    if (vkGetSemaphoreWin32HandleKHR(device.device, &info, &handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to export semaphore");
    }
    
    return handle;
}

HANDLE Scatter::getDoneSemaphoreHandle() {
    auto vkGetSemaphoreWin32HandleKHR = PFN_vkGetSemaphoreWin32HandleKHR(vkGetDeviceProcAddr(device.device, "vkGetSemaphoreWin32HandleKHR"));

    HANDLE handle;

    VkSemaphoreGetWin32HandleInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
    info.semaphore = doneSemaphore;
    info.handleType = VkExternalSemaphoreHandleTypeFlagBits::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    if (vkGetSemaphoreWin32HandleKHR(device.device, &info, &handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to export semaphore");
    }

    return handle;
}

void Scatter::submit(uint32_t width, uint32_t height) {
    VkPhysicalDeviceRayTracingPropertiesNV rtProps{};
    rtProps.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;

    VkPhysicalDeviceProperties2 pdProps{};
    pdProps.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    pdProps.pNext = &rtProps;

    vkGetPhysicalDeviceProperties2(device.physicalDevice, &pdProps);

    auto commandBuffer = device.createCommandBuffer();

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    rtx.execute(device.device, commandBuffer, width, height, rtProps);

    vkEndCommandBuffer(commandBuffer);

    // incase we're submitting faster then rendering, we wait for 
    // previous execution to finish before proceeding with  the submit
    vkWaitForFences(device.device, 1, &cpuFence, VK_TRUE, UINT64_MAX);

    vkResetFences(device.device, 1, &cpuFence);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &readySemaphore;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &doneSemaphore;

    if (vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, cpuFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer! \n");
    }
}

void Scatter::createTextures(uint32_t width, uint32_t height) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &memoryProperties);
    rtx.createImages(device.device, { width, height }, &memoryProperties);
    rtx.updateImages(device.device);
}

void Scatter::destroyTextures() {
    rtx.destroyImages(device.device);
}

uint64_t Scatter::addMesh(void* vertices, void* indices, unsigned int vertexCount, unsigned int indexCount) {
    auto& mesh = meshes.emplace_back(vertices, indices, vertexCount, indexCount);

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    vertexBuffer.init(device, vertices, attribDesc.vertexStride * vertexCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    uint32_t indexSize = 0;
    switch (attribDesc.indexFormat) {
        case IndexFormat::UINT32: indexSize = sizeof(uint32_t); break;
        case IndexFormat::UINT16: indexSize = sizeof(uint16_t); break;
    }

    indexBuffer.init(device, indices, indexSize * indexCount, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    VkGeometryNV geometry{};
    VkGeometryTrianglesNV& triangle = geometry.geometry.triangles;

    geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
    triangle.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
    triangle.vertexData = vertexBuffer.getBuffer();
    triangle.vertexOffset = attribDesc.vertexOffset;
    triangle.vertexCount = mesh.vertexCount;
    triangle.vertexStride = attribDesc.vertexStride;
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

    bottomLevels.emplace_back().init(device.device, device.allocator, &BLAScreateInfo);
    bottomLevels.back().record(device, &BLAScreateInfo);

    vertexBuffer.destroy(device);
    indexBuffer.destroy(device);

    return bottomLevels.back().handle;
}

void Scatter::addInstance(uint64_t handle, float* transform) {
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

void Scatter::build() {
    // create top level acceleration structure
    VkAccelerationStructureCreateInfoNV TLAScreateInfo{};
    TLAScreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    TLAScreateInfo.info.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    TLAScreateInfo.info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    TLAScreateInfo.info.instanceCount = static_cast<uint32_t>(instances.size());

    TLAS.init(device.device, device.allocator, &TLAScreateInfo);
    TLAS.record(device, instances.data(), &TLAScreateInfo);
    rtx.updateTLAS(device.device, TLAS.as);
}

void Scatter::clear() {
    TLAS.destroy(device.device, device.allocator);
    for (auto& blas : bottomLevels) {
        blas.destroy(device.device, device.allocator);
    }
}

HANDLE Scatter::getDepthTextureMemoryhandle() {
    return rtx.getDepthTextureMemoryHandle(device.device);
}

Scatter::~Scatter() {
    vkDeviceWaitIdle(device.device);

    rtx.destroy(device.device, device.allocator, device.descriptorPool);
    shaderManager.destroy();
    for (auto& blas : bottomLevels) {
        blas.destroy(device.device, device.allocator);
    }

    TLAS.destroy(device.device, device.allocator);

    vkDestroyFence(device.device, cpuFence, nullptr);

    vkDestroySemaphore(device.device, readySemaphore, nullptr);
    vkDestroySemaphore(device.device, doneSemaphore, nullptr);
}

}