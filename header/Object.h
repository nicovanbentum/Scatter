#pragma once

#include "Vertex.h"

namespace scatter {

struct Object {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    uint32_t indexOffset = 0;
    uint32_t vertexOffset = 0;

    glm::mat4 model = glm::mat4(1.0f);
};

}
