#pragma once

#include "pch.h"
#include "Extensions.h"

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
    // TODO: figure this friend class mess out
    friend class VulkanApplication;
    friend class VulkanSwapchain;
    friend class VulkanBuffer;
    friend class AccelerationStructureBuilder;
    friend struct BottomLevelAS;
    friend struct TopLevelAS;
public:
    void init(GLFWwindow* window);
    ~VulkanDevice();

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer buffer);

private:
    VkDevice device;
    VkInstance instance;
    VkSurfaceKHR surface;
    VmaAllocator allocator;
    VkPhysicalDevice physicalDevice;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkQueue presentQueue;
    VkQueue graphicsQueue;


    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkDescriptorPool descriptorPool;

    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    void createCommandPool();
    void createCommandBuffers();
    std::tuple<VkBuffer, VmaAllocation, VmaAllocationInfo> createStagingBuffer(size_t sizeInBytes);
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
    void createDescriptorPool();

};

}