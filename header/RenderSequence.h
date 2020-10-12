#pragma once

#include "ShaderManager.h"
#include "Swapchain.h"
#include "VertexBuffer.h"
#include "Object.h"

namespace scatter {

class VulkanRenderSequence {
public:
    struct Uniforms {
        glm::mat4 model         = glm::mat4(1.0f);
        glm::mat4 view          = glm::mat4(1.0f);
        glm::mat4 projection    = glm::mat4(1.0f);
    } uniforms;

public:
    void init(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager);
    void destroyFramebuffers(VkDevice device);
    void destroy(VkDevice device, VmaAllocator allocator);

    void createRenderPass(VkDevice device, const VulkanSwapchain& swapchain);
    void createGraphicsPipeline(VkDevice device, VkDescriptorPool descriptorPool, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager);
    void createFramebuffers(VkDevice device, const std::vector<VkImageView>& imageViews, VkExtent2D extent);
    void createDescriptorSets(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool);
    void updateDescriptorSet(VkDevice device, VmaAllocator allocator);

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

    std::vector<VkFramebuffer> framebuffers;
    VkRenderPass renderPass;

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
};
}