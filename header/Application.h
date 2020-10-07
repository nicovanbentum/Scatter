#pragma once

#include "pch.h"

#include "Device.h"
#include "ShaderManager.h"
#include "CommandBufferManager.h"
#include "PipelineManager.h"
#include "Swapchain.h"

namespace scatter {

    class VulkanApplication {
    public:
        VulkanApplication(uint32_t width, uint32_t height);
        ~VulkanApplication();

        void update(float dt);

        VkPipeline createPipeline();

        bool frameBufferResized = false;
    
    private:
        VulkanDevice device;
        VulkanSwapchain swapchain;
        VulkanShaderManager shaderManager;
        CommandBufferManager commandBufferManager;
        VulkanPipelineManager pipelineManager;

        GLFWwindow* window;
        VkSurfaceKHR surface;
        VkSemaphore semaphore;
        VkFence fence;

    };

}