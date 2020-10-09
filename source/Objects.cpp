#include "pch.h"
#include "Objects.h"
#include "VulkanBuffer.h"

namespace scatter {

void Object::init(VulkanDevice& device, CommandBufferManager& commandBufferManager) {
    vertexBuffer.init(device, commandBufferManager, vertices.data(), sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer.init(device, commandBufferManager, indices.data(), sizeof(uint16_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void Object::destroy(const VulkanDevice& device) {
    vertexBuffer.destroy(device);
    indexBuffer.destroy(device);
}

}