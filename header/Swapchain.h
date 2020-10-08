#pragma once

#include "pch.h"
#include "Device.h"

namespace scatter {

class VulkanSwapchain {
    friend class VulkanRenderSequence;
    friend class VulkanApplication;

public:
    void init(GLFWwindow* window, VulkanDevice& device);
    void destroy(VkDevice device);


private:
    VkSwapchainKHR swapChain;
    VkExtent2D swapChainExtent;
    VkFormat swapChainImageFormat;

    std::vector<VkImageView> swapChainImageViews;

    SwapChainSupportDetails querySwapChainSupport(VulkanDevice& device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);
};

}