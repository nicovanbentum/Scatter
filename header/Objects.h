#pragma once

#include "Device.h"
#include "CommandBufferManager.h"

namespace scatter {

class Object {
    friend class CommandBufferManager;

public:
    void init(VulkanDevice& device, CommandBufferManager& commandBufferManager);
    void destroy(const VulkanDevice& device);

    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
private:

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
};

}
