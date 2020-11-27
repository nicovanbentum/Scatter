#pragma once

#include "Device.h"
#include "Object.h"
#include "Swapchain.h"
#include "ShaderManager.h"
#include "AccelStructure.h"
#include "RenderSequence.h"

namespace scatter {

class VulkanApplication {
public:
    void init(uint32_t width, uint32_t height);
    void update(float dt);
    void destroy();

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
    RayTracedShadowsSequence shadowSequence;
    VulkanShaderManager shaderManager;

    std::vector<Object> objects;
    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    BottomLevelAS bottomLevelAS;
    TopLevelAS topLevelAS;

    std::vector<VkSemaphore> imageAvailableSemaphore;
    std::vector<VkSemaphore> renderFinishedSemaphore;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    GLFWwindow* window;
    VkSurfaceKHR surface;
};

}