#include "pch.h"
#include "Objects.h"
#include "VulkanBuffer.h"

namespace scatter {

void Object::init(VulkanDevice& device,const std::vector<Vertex>& vertices,const std::vector<uint16_t>& indices) {
    vertexBuffer.init(device, vertices.data(), sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer.init(device, indices.data(), sizeof(uint16_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    this->vertices = vertices;
    this->indices = indices;
}


void Object::destroy(const VulkanDevice& device) {
    vertexBuffer.destroy(device);
    indexBuffer.destroy(device);
}

}