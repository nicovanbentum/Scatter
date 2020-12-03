#include "pch.h"
#include "Scatter.h"
#include "..\header\Scatter.h"

namespace scatter {

Scatter::Scatter() {
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

void Scatter::createTextures(uint32_t width, uint32_t height) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &memoryProperties);
    rtx.createImages(device.device, { width, height }, &memoryProperties);
    rtx.updateImages(device.device);
}

void Scatter::destroyTextures() {
    rtx.destroyImages(device.device);
}

HANDLE Scatter::getDepthTextureMemoryhandle() {
    return rtx.getDepthTextureMemoryHandle(device.device);
}

Scatter::~Scatter() {
    rtx.destroy(device.device, device.allocator, device.descriptorPool);
    shaderManager.destroy();
}

}