#pragma once

#include "pch.h"
#include "Swapchain.h"

namespace scatter {

    class VulkanDevice {
    public:
        VkRenderPass createRenderPass(VulkanSwapchain swapchain);
        VkFramebuffer createFramebuffer(const std::vector<VkImageView>& attachments);

    private:
        VkDevice device;
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
    };

}