#pragma once

#include "ShaderManager.h"
#include "Swapchain.h"
#include "VulkanBuffer.h"
#include "Object.h"

#include <string.h>

namespace scatter {

class VulkanRenderSequence {
public:
    struct Uniforms {
        glm::mat4 view          = glm::mat4(1.0f);
        glm::mat4 projection    = glm::mat4(1.0f);
    } uniforms;

public:
    void init(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager);
    void destroyFramebuffers(VkDevice device);
    void destroy(VkDevice device, VmaAllocator allocator);
    void destroyDepthTexture(VkDevice device, VmaAllocator allocator);

    void createRenderPass(VkDevice device, const VulkanSwapchain& swapchain);
    void createGraphicsPipeline(VkDevice device, VkDescriptorPool descriptorPool, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager);
    void createFramebuffers(VkDevice device, const std::vector<VkImageView>& imageViews, VkExtent2D extent);
    void createDescriptorSets(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool);
    void updateDescriptorSet(VkDevice device, VmaAllocator allocator);
    void createDepthTexture(VkDevice device, VmaAllocator allocator, const VulkanSwapchain& swapchain);

    void recordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer, VmaAllocator allocator, VkExtent2D extent, VkBuffer vertexBuffer, VkBuffer indexBuffer, const std::vector<Object>& objects, size_t framebufferIndex);

    size_t getFramebuffersCount() { return framebuffers.size(); }

private:
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    // descriptor objects
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
    VkWriteDescriptorSet writeDescriptorSet;
    VkDescriptorBufferInfo descriptorBufferInfo;

    // uniform buffer for descriptor
    VkBuffer uniformBuffer;
    VmaAllocation uniformBufferAlloc;
    VmaAllocationInfo uniformBufferAllocInfo;

    VkImage depthImage;
    VkImageView depthImageView;
    VmaAllocation depthImageAlloc;
    VmaAllocationInfo depthImageAllocInfo;

    std::vector<VkFramebuffer> framebuffers;
    VkRenderPass renderPass;

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
};

class RayTracedShadowsSequence {
public:
    void createShadowImage(VkDevice device, VmaAllocator allocator, uint32_t width, uint32_t height) {
        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.extent = { width, height, 1 };
        imageCreateInfo.format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
        imageCreateInfo.imageType = VkImageType::VK_IMAGE_TYPE_2D;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT;
        imageCreateInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        if (vmaCreateImage(allocator, &imageCreateInfo, &allocCreateInfo, &image, &imageAlloc, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow image");
        } else {
            std::puts("created shadow texture");
        }

        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = image;
        viewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewCreateInfo, nullptr, &shadowImageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow image view");
        }
    }

    void createDescriptorSets(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool, VkAccelerationStructureNV tlas) {
        // allocate the descriptor set
        VkDescriptorSetAllocateInfo descriptorAllocInfo{};
        descriptorAllocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorAllocInfo.descriptorPool = descriptorPool;
        descriptorAllocInfo.descriptorSetCount = 1;
        descriptorAllocInfo.pSetLayouts = &descriptorSetLayout;

        if (vkAllocateDescriptorSets(device, &descriptorAllocInfo, &descriptorSet)) {
            throw std::runtime_error("failed to allocate descriptor sets");
        } else {
            std::puts("Succesfully allocated descriptorSets!!");
        }

        // AS write set
        VkWriteDescriptorSetAccelerationStructureNV write = {};
        write.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
        write.accelerationStructureCount = 1;
        write.pAccelerationStructures = &tlas;

        VkWriteDescriptorSet writeAS = {};
        writeAS.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeAS.dstBinding = 0;
        writeAS.descriptorCount = 1;
        writeAS.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
        writeAS.dstSet = descriptorSet;
        writeAS.pNext = &write;

        // image write set
        VkDescriptorImageInfo imgInfo = {};
        imgInfo.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
        imgInfo.imageView = shadowImageView;

        VkWriteDescriptorSet imgWriteSet = {};
        imgWriteSet.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        imgWriteSet.dstBinding = 1;
        imgWriteSet.descriptorCount = 1;
        imgWriteSet.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        imgWriteSet.dstSet = descriptorSet;
        imgWriteSet.pImageInfo = &imgInfo;

        std::array< VkWriteDescriptorSet, 2> sets = { writeAS, imgWriteSet };
        vkUpdateDescriptorSets(device, 2, sets.data(), 0, nullptr);
    }

    void createPipeline(VkDevice device, VkDescriptorPool descriptorPool, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager) {
        VkDescriptorSetLayoutBinding TLASbinding = {};
        TLASbinding.binding = 0;
        TLASbinding.descriptorCount = 1;
        TLASbinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;;
        TLASbinding.pImmutableSamplers = nullptr;
        TLASbinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

        VkDescriptorSetLayoutBinding outputImageBinding = {};
        outputImageBinding.binding = 1;
        outputImageBinding.descriptorCount = 1;
        outputImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        outputImageBinding.pImmutableSamplers = nullptr;
        outputImageBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { TLASbinding, outputImageBinding };

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetLayoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create RTX descriptor set layout");
        } else {
            std::puts("Created descriptor set layout");
        }
    }



private:
    // descriptor set stuff
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
    VkWriteDescriptorSet writeDescriptorSet;
    VkDescriptorBufferInfo descriptorBufferInfo;

    VkImage image;
    VmaAllocation imageAlloc;
    VkImageView shadowImageView;
};

}