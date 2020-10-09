#pragma once

#include "pch.h"

#include "Device.h"
#include "ShaderManager.h"
#include "PipelineManager.h"
#include "Swapchain.h"

namespace scatter {

class VulkanApplication {
public:
    void init(uint32_t width, uint32_t height);
    void destroy();

    void update(float dt);

    void drawFrame();
    void createSyncObjects();
    void recreateSwapChain();

    bool frameBufferResized = false;

private:
    const int MAX_FRAME_IN_FLIGHT = 2;
    size_t currentFrame = 0;

    VulkanDevice device;
    VulkanSwapchain swapchain;
    VulkanRenderSequence renderSequence;
    VulkanShaderManager shaderManager;
    VulkanPipelineManager pipelineManager;
    VulkanVertexBuffer vertexBuffer;

    std::vector<VkSemaphore> imageAvailableSemaphore;
    std::vector<VkSemaphore> renderFinishedSemaphore;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    GLFWwindow* window;
    VkSurfaceKHR surface;
};

}