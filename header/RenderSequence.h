#pragma once

#include "ShaderManager.h"
#include "Swapchain.h"
#include "VulkanBuffer.h"
#include "Object.h"
#include "Texture.h"
#include "Util.h"

namespace scatter {

class VulkanRenderSequence {
public:
    struct Uniforms {
        glm::mat4 view          = glm::mat4(1.0f);
        glm::mat4 projection    = glm::mat4(1.0f);
    } uniforms;

public:
    void init(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager, VkImageView depthView);
    void destroyFramebuffers(VkDevice device);
    void destroy(VkDevice device, VmaAllocator allocator);

    void createRenderPass(VkDevice device, const VulkanSwapchain& swapchain);
    void createGraphicsPipeline(VkDevice device, VkDescriptorPool descriptorPool, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager);
    void createFramebuffers(VkDevice device, const std::vector<VkImageView>& imageViews, VkExtent2D extent, VkImageView depthView);
    void createDescriptorSets(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool);
    void updateDescriptorSet(VkDevice device, VmaAllocator allocator);

    void execute(VkDevice device, VkCommandBuffer commandBuffer, VmaAllocator allocator, VkExtent2D extent, VkBuffer vertexBuffer, VkBuffer indexBuffer, const std::vector<Object>& objects, size_t framebufferIndex);

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

    std::vector<VkFramebuffer> framebuffers;
    VkRenderPass renderPass;

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
};

class RayTracedShadowsSequence {
public:
    struct {
        glm::vec4 lightDirection = { 0, -1, 0, 1.0 };
        glm::mat4 inverseViewProjection = glm::mat4(1.0f);
    } pushData;

    HANDLE getMemoryHandle(VkDevice device, VkDeviceMemory memory);

    void init(VkDevice device, VmaAllocator allocator, VkPhysicalDevice pdevice, VulkanShaderManager& shaderManager, VkExtent2D extent);
    void destroy(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool);

    void createImages(VkDevice device, VkExtent2D extent, VkPhysicalDeviceMemoryProperties* memProperties);
    void updateImages(VkDevice device);
    void destroyImages(VkDevice device);

    void createDescriptorSets(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool, VkAccelerationStructureNV tlas);
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
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
    VkWriteDescriptorSet writeDescriptorSet;
    VkDescriptorBufferInfo descriptorBufferInfo;

    // shader binding table
    VkBuffer sbtBuffer;
    VmaAllocation sbtAlloc;
    std::vector<VkRayTracingShaderGroupCreateInfoNV> groups;
};

}