#pragma once

#include "pch.h"
#include "Shader.h"

namespace scatter {

class VulkanShaderManager {
public:
    void init(VkDevice device);
    void destroy();

    VkShaderModule getShader(const std::filesystem::path& path);
    VkShaderModule addShader(const std::filesystem::path& path);
    bool compile(const std::string& filenameIn, const std::string& filenameOut);

private:
    VkDevice device;
    std::vector<char> readFromSpirv(const std::filesystem::path& pathName);
    std::unordered_map<std::string, VkShaderModule> shaders;
};

}