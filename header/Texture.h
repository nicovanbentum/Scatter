#pragma once

#include "pch.h"

namespace scatter {

class VulkanTexture {
public:
    bool loadFromDisk(const std::string& pathName);

private:
    VkImage image;
    VkImageView view;
    VkSampler sampler;
};

}