#pragma once

#include "ShaderManager.h"
#include "VulkanBuffer.h"
#include "Texture.h"
#include "Util.h"

namespace scatter {

class RayTracedShadowsSequence {
public:
    struct {
        glm::vec4 lightDirection = { 0, -1, 0, 1.0 };
        glm::mat4 inverseViewProjection = glm::mat4(1.0f);
    } pushData;

    HANDLE getMemoryHandle(VkDevice device, VkDeviceMemory memory);

    HANDLE getDepthTextureMemoryHandle(VkDevice device);
    HANDLE getShadowTextureMemoryHandle(VkDevice device);

    void init(VkDevice device, VmaAllocator allocator, VkPhysicalDevice pdevice, VulkanShaderManager& shaderManager);
    void destroy(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool);

    void createImages(VkDevice device, VkExtent2D extent, VkPhysicalDeviceMemoryProperties* memProperties);
    void updateImages(VkDevice device);
    void destroyImages(VkDevice device);

    void updateTLAS(VkDevice device, VkAccelerationStructureNV tlas);

    void createDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool);
    void createPipeline(VkDevice device, VulkanShaderManager& shaderManager);
    void createSbtTable(VkDevice device, VmaAllocator allocator, const VkPhysicalDeviceRayTracingPropertiesNV& rtProps);

    void execute(VkDevice device, VkCommandBuffer cmdBuffer, uint32_t width, uint32_t height, const VkPhysicalDeviceRayTracingPropertiesNV& rtProps);

    // textures
    TextureEXT depthTexture;
    TextureEXT shadowsTexture;
private:
    // pipeline stuff
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    // descriptor set stuff
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout;
    VkWriteDescriptorSet writeDescriptorSet;
    VkDescriptorBufferInfo descriptorBufferInfo;

    // shader binding table
    VkBuffer sbtBuffer;
    VmaAllocation sbtAlloc;
    std::vector<VkRayTracingShaderGroupCreateInfoNV> groups;
};

}