#pragma once

#include "pch.h"

namespace scatter {

    class VulkanShader {
    public:
        VulkanShader();
        ~VulkanShader();

    private:
        VkShaderModule module;
    };

}