#pragma once

#include "Device.h"

namespace scatter {

class Object {
    friend class VulkanApplication;

public:
    void init(VulkanDevice& device,const std::vector<Vertex>& vertices,const std::vector<uint16_t>& indices);
    void destroy(const VulkanDevice& device);

    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
private:
    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
};

}
