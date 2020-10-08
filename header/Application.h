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
        void init(uint32_t width, uint32_t height);
        void destroy();

        void update(float dt);

        bool frameBufferResized = false;
    
    private:
        VulkanDevice device;
        VulkanSwapchain swapchain;
        VulkanRenderSequence renderSequence;
        VulkanShaderManager shaderManager;
        CommandBufferManager commandBufferManager;
        VulkanPipelineManager pipelineManager;
        VulkanVertexBuffer vertexBuffer;

        GLFWwindow* window;
        VkSurfaceKHR surface;
        VkSemaphore semaphore;
        VkFence fence;

    };

}