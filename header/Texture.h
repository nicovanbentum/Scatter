#pragma once

namespace scatter {

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

public:
    VkImage image;
    VkDeviceMemory memory;

    // opt
    VkImageView view;
    VkSampler sampler;
};

}