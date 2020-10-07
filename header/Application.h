#pragma once

#include "pch.h"

#include "Device.h"
#include "ShaderManager.h"
#include "CommandBufferManager.h"
#include "PipelineManager.h"

namespace scatter {

    class VulkanApplication {
    public:
        VulkanApplication();
        ~VulkanApplication();

        void update(float dt);

        VkPipeline createPipeline();

    private:
        VulkanDevice device;
        VulkanShaderManager shaderManager;
        CommandBufferManager commandBufferManager;
        VulkanPipelineManager pipelineManager;

        VkDebugUtilsMessengerEXT debugMessenger;

        GLFWwindow* window;
        VkSurfaceKHR surface;
        VkSemaphore semaphore;
        VkFence fence;

    };

}