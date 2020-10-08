#pragma once

#include "pch.h"
#include "Device.h"
#include "Swapchain.h"
#include "RenderSequence.h"
#include "VertexBuffer.h"

namespace scatter {

class CommandBufferManager {
    friend class VulkanApplication;
public:
    void init(VulkanDevice& vulkanDevice);
    void destroy(VkDevice device);

    VkCommandBuffer beginSingleTimeCommands(VulkanDevice& device);
    void endSingleTimeCommands(VulkanDevice& device, VkCommandBuffer buffer);

    void createCommandPool(VulkanDevice& vulkanDevice);
    void recordCommandBuffer(const VkDevice device, VulkanRenderSequence& renderSequence, VkExtent2D extent, VulkanVertexBuffer& vertexBuffer);

private:
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
};

}