#pragma once

#include "pch.h"

namespace scatter {

    class VulkanApplication {
    public:
        VulkanApplication();
        ~VulkanApplication();

        void update(float dt);

        VkPipeline createPipeline();

    private:
        VulkanDevice device;
        VulkanShaderManager;
        CommandBufferManager;
        VulkanPipelineManager;

        VkDebugUtilsMessengerEXT debugMessenger;

        GLFWwindow* window;
        VkSurfaceKHR surface;
        VkSemaphore semaphore;
        VkFence fence;

    };

}