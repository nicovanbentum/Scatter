#pragma once

namespace scatter {

struct Vertex {
    glm::vec3 pos;
    glm::vec2 texcoord;
    glm::vec3 normal;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

}