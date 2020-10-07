#pragma once

#include "pch.h"
#include "RenderSequence.h"

namespace scatter {

    class VulkanPipelineManager {
    public:
        VkPipeline getPipeline();
        VkPipeline createPipeline(VulkanRenderSequence renderSequence);

    private:
        std::vector<VkPipeline> pipelines;
    };

}