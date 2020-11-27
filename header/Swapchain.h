#pragma once

#include "Device.h"

namespace scatter {

class VulkanSwapchain {
    friend class VulkanRenderSequence;
    friend class VulkanApplication;

public:
    void init(GLFWwindow* window, VulkanDevice& device);
    void destroy(VkInstance instance, VkDevice device);


private:
    VkQueue presentQueue;
    VkSurfaceKHR surface;
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