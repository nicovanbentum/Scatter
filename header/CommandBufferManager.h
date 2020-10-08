#pragma once

#include "pch.h"
#include "Device.h"
#include "Swapchain.h"
#include "RenderSequence.h"
#include "VertexBuffer.h"

namespace scatter {

    class CommandBufferManager {

    public:
        void init(VulkanDevice& vulkanDevice,VulkanRenderSequence& renderSequence,VkExtent2D& extent,VulkanVertexBuffer& vertexBuffer);
        void destroy(VkDevice device);

        VkCommandPool createCommandPool(VulkanDevice& vulkanDevice);
        VkCommandBuffer recordCommandBuffer(const VkDevice device, VulkanRenderSequence& renderSequence, VkExtent2D extent, VulkanVertexBuffer& vertexBuffer);

    private:
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;
    };

}