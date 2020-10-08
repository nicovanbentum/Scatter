#pragma once

#include "pch.h"

namespace scatter {

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete();
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentMode;
    };

    class VulkanDevice {
        friend class VulkanApplication;
        friend class VulkanSwapchain;
        friend class CommandBufferManager;
        friend class VulkanVertexBuffer;

    public:
        void init(GLFWwindow* window);
        ~VulkanDevice();

    private:
        VkDevice device;
        VkInstance instance;
        VkSurfaceKHR surface;
        VmaAllocator allocator;
        VkPhysicalDevice physicalDevice;
        VkDebugUtilsMessengerEXT debugMessenger;

        VkQueue presentQueue;
        VkQueue graphicsQueue;

        const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
        const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        void createInstance();
        void createSurface(GLFWwindow* window);
        void pickPhysicalDevice();
        void createLogicalDevice();
        bool isDeviceSuitable(VkPhysicalDevice device);
        void setupDebugMessenger();
        std::vector<const char*> getRequiredExtensions();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    };

}