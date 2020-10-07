#pragma once

#include "pch.h"

namespace scatter {
    class VulkanShaderManager {
    public:
        VkShaderModule getShader(const std::string& name);

    private:
        std::vector<char> readFromSpirv(const std::string& pathName);
        bool compile(const std::string& filenameIn, const std::string& filenameOut);
    };
}