#pragma once

#include "pch.h"

namespace scatter {

inline uint32_t getMemoryIndex(VkPhysicalDeviceMemoryProperties* physicalProperties, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < physicalProperties->memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (physicalProperties->memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

/*
    Textures in Scatter are purely used for rendering operations
*/
struct TextureCreateInfo {
    VkExtent2D extent;
    VkFormat format;
    VkImageUsageFlags usage;
};

class TextureEXT {
public:
    TextureEXT();
    TextureEXT(VkDevice device, TextureCreateInfo* info, VkPhysicalDeviceMemoryProperties* properties);

    VkImageView createView(VkDevice device, TextureCreateInfo* info, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    VkSampler createSampler(VkDevice device);
    HANDLE getMemoryHandle(VkDevice device, VkDeviceMemory memory);
    
    void destroy(VkDevice device);

public: // in Vulkan style, we keep things accessible
    VkImage image;
    VkDeviceMemory memory;

    // opt
    VkImageView view;
    VkSampler sampler;
};

}