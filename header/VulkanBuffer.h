#pragma once
#include "pch.h"
#include "Device.h"
#include "CommandBufferManager.h"

namespace scatter {

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    inline static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    inline static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescription{};
        attributeDescription[0].binding = 0;
        attributeDescription[0].location = 0;
        attributeDescription[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription[0].offset = offsetof(Vertex, pos);

        attributeDescription[1].binding = 0;
        attributeDescription[1].location = 1;
        attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription[1].offset = offsetof(Vertex, color);
        return attributeDescription;
    }
};

class VulkanBuffer {
    friend class CommandBufferManager;

public:
    void init(VulkanDevice& device, CommandBufferManager& commandBufferManager, const void* vectorData, size_t sizeOfData, VkBufferUsageFlagBits usageFlag);
    void destroy(const VulkanDevice& device);

private:
    VkBuffer buffer;
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
};

}
