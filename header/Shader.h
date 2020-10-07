#pragma once

#include "pch.h"

namespace scatter {

    class VulkanShader {
        friend class VulkanShaderManager;

    public:
        VulkanShader();
        ~VulkanShader();

    private:
        VkShaderModule module;
    };

}