#pragma once

#include "ShaderManager.h"
#include "Swapchain.h"
#include "VulkanBuffer.h"
#include "Object.h"
#include "Texture.h"

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

    VkImageView depthImageView;
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
    VmaAllocation depthImageAlloc;
    VmaAllocationInfo depthImageAllocInfo;

    std::vector<VkFramebuffer> framebuffers;
    VkRenderPass renderPass;

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
};

inline uint32_t findMemoryType(VkPhysicalDevice GPU, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(GPU, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

class RayTracedShadowsSequence {
public:
    struct {
        glm::vec3 lightDirection = { 0, -1, 0 };
        glm::mat4 inverseViewProjection = glm::mat4(1.0f);
    } pushData;

    HANDLE getMemoryHandle(VkDevice device, VkDeviceMemory memory) {
        auto vkGetMemoryWin32HandleKHR = PFN_vkGetMemoryWin32HandleKHR(vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR"));
        
        VkMemoryGetWin32HandleInfoKHR handleInfo = {};
        handleInfo.sType        = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        handleInfo.handleType   = VkExternalMemoryHandleTypeFlagBits::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
        handleInfo.memory       = memory;

        HANDLE handle;
        vkGetMemoryWin32HandleKHR(device, &handleInfo, &handle);
        return handle;
    }

    void createImages(VkDevice device, VkExtent2D extent, VkPhysicalDeviceMemoryProperties* memProperties) {
        TextureCreateInfo depthTextureInfo = {};
        depthTextureInfo.extent = extent;
        depthTextureInfo.format = VK_FORMAT_D32_SFLOAT;
        depthTextureInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        depthTexture = TextureEXT(device, &depthTextureInfo, memProperties);
        depthTexture.createView(device, &depthTextureInfo, VK_IMAGE_ASPECT_DEPTH_BIT);
        depthTexture.createSampler(device);

        TextureCreateInfo shadowTextureInfo = {};
        shadowTextureInfo.extent = extent;
        shadowTextureInfo.format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
        shadowTextureInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;

        shadowsTexture = TextureEXT(device, &shadowTextureInfo, memProperties);
        shadowsTexture.createView(device, &shadowTextureInfo);
    }

    void createDescriptorSets(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool, VkAccelerationStructureNV tlas) {
        // allocate the descriptor set
        VkDescriptorSetAllocateInfo descriptorAllocInfo{};
        descriptorAllocInfo.descriptorSetCount  = 1;
        descriptorAllocInfo.descriptorPool      = descriptorPool;
        descriptorAllocInfo.pSetLayouts         = &descriptorSetLayout;
        descriptorAllocInfo.sType               = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

        if (vkAllocateDescriptorSets(device, &descriptorAllocInfo, &descriptorSet)) {
            throw std::runtime_error("failed to allocate descriptor sets");
        } else {
            std::puts("Succesfully allocated descriptorSets!!");
        }

        // AS write set
        VkWriteDescriptorSetAccelerationStructureNV write = {};
        write.accelerationStructureCount    = 1;
        write.pAccelerationStructures       = &tlas;
        write.sType                         = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;

        VkWriteDescriptorSet writeAS = {};
        writeAS.dstBinding          = 0;
        writeAS.descriptorCount     = 1;
        writeAS.pNext               = &write;
        writeAS.dstSet              = descriptorSet;
        writeAS.sType               = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeAS.descriptorType      = VkDescriptorType::VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

        // image write set
        VkDescriptorImageInfo shadowDescriptorImage = {};
        shadowDescriptorImage.imageView       = shadowsTexture.view;
        shadowDescriptorImage.imageLayout     = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet shadowWriteSet = {};
        shadowWriteSet.dstBinding          = 1;
        shadowWriteSet.descriptorCount     = 1;
        shadowWriteSet.pImageInfo          = &shadowDescriptorImage;
        shadowWriteSet.dstSet              = descriptorSet;
        shadowWriteSet.descriptorType      = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        shadowWriteSet.sType               = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

        VkDescriptorImageInfo depthDescriptorImage = {};
        depthDescriptorImage.imageView = depthTexture.view;
        depthDescriptorImage.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
        depthDescriptorImage.sampler = depthTexture.sampler;

        VkWriteDescriptorSet depthWriteSet = {};
        depthWriteSet.dstBinding        = 2;
        depthWriteSet.descriptorCount   = 1;
        depthWriteSet.pImageInfo        = &depthDescriptorImage;
        depthWriteSet.dstSet            = descriptorSet;
        depthWriteSet.descriptorType    = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        depthWriteSet.sType             = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

        std::array< VkWriteDescriptorSet, 3> sets = { writeAS, shadowWriteSet, depthWriteSet };
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    }

    void createPipeline(VkDevice device, VkDescriptorPool descriptorPool, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager) {
        //// shader stages ////
        VkPipelineShaderStageCreateInfo raygenShaderInfo{};
        raygenShaderInfo.pName = "main";
        raygenShaderInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_NV;
        raygenShaderInfo.module = shaderManager.getShader("shader/raytrace.rgen.spv");
        raygenShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

        VkPipelineShaderStageCreateInfo missShaderInfo{};
        missShaderInfo.pName = "main";
        missShaderInfo.stage = VK_SHADER_STAGE_MISS_BIT_NV;
        missShaderInfo.module = shaderManager.getShader("shader/raytrace.rmiss.spv");
        missShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { raygenShaderInfo, missShaderInfo };

        //// descriptor set bindings ////
        VkDescriptorSetLayoutBinding TLASbinding = {};
        TLASbinding.binding = 0;
        TLASbinding.descriptorCount = 1;
        TLASbinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
        TLASbinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

        VkDescriptorSetLayoutBinding outputImageBinding = {};
        outputImageBinding.binding = 1;
        outputImageBinding.descriptorCount = 1;
        outputImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        outputImageBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

        VkDescriptorSetLayoutBinding inputImageBinding = {};
        inputImageBinding.binding = 2;
        inputImageBinding.descriptorCount = 1;
        inputImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        inputImageBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

        std::array<VkDescriptorSetLayoutBinding, 3> bindings = { TLASbinding, outputImageBinding,  inputImageBinding };

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetLayoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create RTX descriptor set layout");
        } else {
            std::puts("Created descriptor set layout");
        }

        //// create the pipeline layout ////
        VkPushConstantRange pcr{};
        pcr.offset = 0;
        pcr.size = sizeof(pushData);
        pcr.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

        VkPipelineLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCreateInfo.setLayoutCount = 1;
        layoutCreateInfo.pSetLayouts = &descriptorSetLayout;
        layoutCreateInfo.pushConstantRangeCount = 1;
        layoutCreateInfo.pPushConstantRanges = &pcr;

        if (vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout! \n");
        } else {
            std::cout << "successfully created pipeline layout! \n";
        }

        /// define the groups, a miss group and raygen group
        VkRayTracingShaderGroupCreateInfoNV group = {};
        group.generalShader         = 0;
        group.anyHitShader          = VK_SHADER_UNUSED_NV;
        group.closestHitShader      = VK_SHADER_UNUSED_NV;
        group.intersectionShader    = VK_SHADER_UNUSED_NV;
        group.sType                 = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
        group.type                  = VkRayTracingShaderGroupTypeNV::VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;

        groups.push_back(group);
        group.generalShader = 1;
        groups.push_back(group);

        VkRayTracingPipelineCreateInfoNV pipelineInfo = {};
        pipelineInfo.basePipelineIndex  = 0;
        pipelineInfo.maxRecursionDepth  = 1;
        pipelineInfo.pGroups            = groups.data();
        pipelineInfo.layout             = pipelineLayout;
        pipelineInfo.pStages            = shaderStages.data();
        pipelineInfo.groupCount         = static_cast<uint32_t>(groups.size());
        pipelineInfo.stageCount         = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.sType              = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;

        if (vk_nv_ray_tracing::vkCreateRayTracingPipelinesNV(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create ray tracing pipeline");
        } else {
            std::puts("sucessfuly created ray tracing pipeline!!");
        }
    }

    void createSbtTable(VkDevice device, VmaAllocator allocator, const VkPhysicalDeviceRayTracingPropertiesNV& rtProps) {
        const uint32_t groupCount = static_cast<uint32_t>(groups.size());
        const uint32_t sbtSize = groupCount * rtProps.shaderGroupBaseAlignment;
        std::vector<uint8_t> shaderHandles(sbtSize);

        if (vk_nv_ray_tracing::vkGetRayTracingShaderGroupHandlesNV(device, pipeline, 0, groupCount, sbtSize, shaderHandles.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to get rt shader group handles");
        }

        VkBufferCreateInfo sbtBufferCreateInfo = {};
        sbtBufferCreateInfo.size = sbtSize;
        sbtBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        sbtBufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        sbtBufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;

        VmaAllocationCreateInfo sbtBufferAllocInfo = {};
        sbtBufferAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        sbtBufferAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo allocInfo{};

        if (vmaCreateBuffer(allocator, &sbtBufferCreateInfo, &sbtBufferAllocInfo, &sbtBuffer, &sbtAlloc, &allocInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to create sbt buffer");
        } else {
            std::puts("creates sbtbuffer!!!");
        }

        auto* pData = reinterpret_cast<uint8_t*>(allocInfo.pMappedData);
        for (uint32_t g = 0; g < groupCount; g++) {
            std::memcpy(pData, groups.data() + g * rtProps.shaderGroupHandleSize, rtProps.shaderGroupHandleSize);
            pData += rtProps.shaderGroupBaseAlignment;
        }
    }

    void destroy(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool) {
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
        
        depthTexture.destroy(device);
        shadowsTexture.destroy(device);

        vmaDestroyBuffer(allocator, sbtBuffer, sbtAlloc);
    }

    void record(VkDevice device, VkCommandBuffer cmdBuffer, uint32_t width, uint32_t height, const VkPhysicalDeviceRayTracingPropertiesNV& rtProps) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to record begin command buffer \n");
        }

        vkCmdBindPipeline(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipeline);
        vkCmdBindDescriptorSets(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
        vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_RAYGEN_BIT_NV, 0, sizeof(pushData), &pushData);
    
        VkDeviceSize progSize = rtProps.shaderGroupBaseAlignment;  // Size of a program identifier
        VkDeviceSize rayGenOffset   = 0u * progSize;  // Start at the beginning of m_sbtBuffer
        VkDeviceSize missOffset     = 1u * progSize;  // Jump over raygen
        VkDeviceSize missStride     = progSize;

        vk_nv_ray_tracing::vkCmdTraceRaysNV(cmdBuffer, 
            sbtBuffer, rayGenOffset, // raygen group 
            sbtBuffer, missOffset, missStride, 
            VK_NULL_HANDLE, 0, 0, 
            VK_NULL_HANDLE, 0, 0, 
            width, height, 1
        );

        if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer! \n");
        }
    }

private:
    // pipeline stuff
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    // descriptor set stuff
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
    VkWriteDescriptorSet writeDescriptorSet;
    VkDescriptorBufferInfo descriptorBufferInfo;

    // textures
    TextureEXT depthTexture;
    TextureEXT shadowsTexture;

    // shader binding table
    VkBuffer sbtBuffer;
    VmaAllocation sbtAlloc;
    std::vector<VkRayTracingShaderGroupCreateInfoNV> groups;
};

}