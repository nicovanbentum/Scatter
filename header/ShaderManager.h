#pragma once

#include <filesystem>

namespace scatter {

class VulkanShaderManager {
public:
    void init(VkDevice device);
    void destroy();

    VkShaderModule getShader(const std::filesystem::path& path);
    bool compile(const std::string& filenameIn, const std::string& filenameOut);

private:
    VkShaderModule addShader(const std::filesystem::path& path);
    std::vector<char> readFromSpirv(const std::filesystem::path& pathName);

private:
    VkDevice device;
    std::unordered_map<std::string, VkShaderModule> shaders;
};

}