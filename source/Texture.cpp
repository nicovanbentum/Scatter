#include "pch.h"
#include "Texture.h"

namespace scatter {

static uint32_t getMemoryIndex(VkPhysicalDeviceMemoryProperties* physicalProperties, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < physicalProperties->memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (physicalProperties->memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

TextureEXT::TextureEXT() : image(nullptr), memory(nullptr), view(nullptr), sampler(nullptr) {}

TextureEXT::TextureEXT(VkDevice device, TextureCreateInfo* info, VkPhysicalDeviceMemoryProperties* properties) : TextureEXT() {
    VkExternalMemoryImageCreateInfo imageInfoEXT = {};
    imageInfoEXT.sType = VkStructureType::VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    imageInfoEXT.handleTypes = VkExternalMemoryHandleTypeFlagBits::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { info->extent.width, info->extent.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = info->format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = info->usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.pNext = &imageInfoEXT;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create depth image");
    }

    std::puts("CREATING VULKAN IMAGE");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = getMemoryIndex(properties, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkExportMemoryAllocateInfo exportMemInfo = {};
    exportMemInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    exportMemInfo.handleTypes = VkExternalMemoryHandleTypeFlagBits::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    allocInfo.pNext = &exportMemInfo;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    if (vkBindImageMemory(device, image, memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("failed to bind memory to texture");
    }
}

VkImageView TextureEXT::createView(VkDevice device, TextureCreateInfo* info, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.image = image;
    viewCreateInfo.format = info->format;
    viewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.layerCount = 1;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

    if (vkCreateImageView(device, &viewCreateInfo, nullptr, &view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow image view");
    }

    return view;
}

VkSampler TextureEXT::createSampler(VkDevice device) {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vk sampler");
    }

    return sampler;
}

HANDLE TextureEXT::getMemoryHandle(VkDevice device, VkDeviceMemory memory) {
    auto vkGetMemoryWin32HandleKHR = PFN_vkGetMemoryWin32HandleKHR(vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR"));

    VkMemoryGetWin32HandleInfoKHR handleInfo = {};
    handleInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    handleInfo.handleType = VkExternalMemoryHandleTypeFlagBits::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    handleInfo.memory = memory;

    HANDLE handle;
    vkGetMemoryWin32HandleKHR(device, &handleInfo, &handle);
    return handle;
}

void TextureEXT::destroy(VkDevice device) {
    vkDestroyImageView(device, view, nullptr);
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, memory, nullptr);
    vkDestroySampler(device, sampler, nullptr);
}

} // scatter