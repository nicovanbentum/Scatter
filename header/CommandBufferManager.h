#pragma once

#include "pch.h"

namespace scatter {

    class CommandBufferManager {
    public:
        VkCommandBuffer recordCommandBuffer();

    private:
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;
    };

}