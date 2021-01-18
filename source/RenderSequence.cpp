#include "pch.h"
#include "RenderSequence.h"

namespace scatter {

static uint32_t findMemoryType(VkPhysicalDevice GPU, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(GPU, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

HANDLE RayTracedShadowsSequence::getMemoryHandle(VkDevice device, VkDeviceMemory memory) {
    auto vkGetMemoryWin32HandleKHR = PFN_vkGetMemoryWin32HandleKHR(vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR"));

    VkMemoryGetWin32HandleInfoKHR handleInfo = {};
    handleInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    handleInfo.handleType = VkExternalMemoryHandleTypeFlagBits::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    handleInfo.memory = memory;

    HANDLE handle;
    if (vkGetMemoryWin32HandleKHR(device, &handleInfo, &handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to get memory win32 handle");
    }
    return handle;
}

HANDLE RayTracedShadowsSequence::getDepthTextureMemoryHandle(VkDevice device) {
    return getMemoryHandle(device, depthTexture.memory);
}

HANDLE RayTracedShadowsSequence::getShadowTextureMemoryHandle(VkDevice device) {
    return getMemoryHandle(device, shadowsTexture.memory);
}

void RayTracedShadowsSequence::createImages(VkDevice device, VkExtent2D extent, VkPhysicalDeviceMemoryProperties* memProperties) {
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

void RayTracedShadowsSequence::destroyImages(VkDevice device) {
    depthTexture.destroy(device);
    shadowsTexture.destroy(device);
}

void RayTracedShadowsSequence::updateTLAS(VkDevice device, VkAccelerationStructureNV tlas) {
    // AS write set
    VkWriteDescriptorSetAccelerationStructureNV write = {};
    write.accelerationStructureCount = 1;
    write.pAccelerationStructures = &tlas;
    write.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;

    VkWriteDescriptorSet writeAS = {};
    writeAS.dstBinding = 0;
    writeAS.descriptorCount = 1;
    writeAS.pNext = &write;
    writeAS.dstSet = descriptorSet;
    writeAS.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeAS.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

    vkUpdateDescriptorSets(device, 1u, &writeAS, 0, nullptr);
}

void RayTracedShadowsSequence::createDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool) {
    // allocate the descriptor set
    VkDescriptorSetAllocateInfo descriptorAllocInfo{};
    descriptorAllocInfo.descriptorSetCount = 1;
    descriptorAllocInfo.descriptorPool = descriptorPool;
    descriptorAllocInfo.pSetLayouts = &descriptorSetLayout;
    descriptorAllocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

    if (vkAllocateDescriptorSets(device, &descriptorAllocInfo, &descriptorSet)) {
        throw std::runtime_error("failed to allocate descriptor sets");
    } else {
        std::puts("Succesfully allocated descriptorSets!!");
    }
}

void RayTracedShadowsSequence::updateImages(VkDevice device) {
    // image write set
    VkDescriptorImageInfo shadowDescriptorImage = {};
    shadowDescriptorImage.imageView = shadowsTexture.view;
    shadowDescriptorImage.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet shadowWriteSet = {};
    shadowWriteSet.dstBinding = 1;
    shadowWriteSet.descriptorCount = 1;
    shadowWriteSet.pImageInfo = &shadowDescriptorImage;
    shadowWriteSet.dstSet = descriptorSet;
    shadowWriteSet.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    shadowWriteSet.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

    VkDescriptorImageInfo depthDescriptorImage = {};
    depthDescriptorImage.imageView = depthTexture.view;
    depthDescriptorImage.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
    depthDescriptorImage.sampler = depthTexture.sampler;

    VkWriteDescriptorSet depthWriteSet = {};
    depthWriteSet.dstBinding = 2;
    depthWriteSet.descriptorCount = 1;
    depthWriteSet.pImageInfo = &depthDescriptorImage;
    depthWriteSet.dstSet = descriptorSet;
    depthWriteSet.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    depthWriteSet.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

    std::array< VkWriteDescriptorSet, 2> sets = { shadowWriteSet, depthWriteSet };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
}

void RayTracedShadowsSequence::createPipeline(VkDevice device, VulkanShaderManager& shaderManager) {
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

    VkPipelineShaderStageCreateInfo hitShaderInfo{};
    hitShaderInfo.pName = "main";
    hitShaderInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
    hitShaderInfo.module = shaderManager.getShader("shader/raytrace.rchit.spv");
    hitShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages = { raygenShaderInfo, missShaderInfo, hitShaderInfo };

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
    group.generalShader = VK_SHADER_UNUSED_NV;
    group.anyHitShader = VK_SHADER_UNUSED_NV;
    group.closestHitShader = VK_SHADER_UNUSED_NV;
    group.intersectionShader = VK_SHADER_UNUSED_NV;
    group.sType = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
    group.type = VkRayTracingShaderGroupTypeNV::VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;

    // raygen
    groups.emplace_back(group).generalShader = 0;
    groups.back().type = VkRayTracingShaderGroupTypeNV::VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;

    // miss
    groups.emplace_back(group).generalShader = 1;
    groups.back().type = VkRayTracingShaderGroupTypeNV::VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;

    // closest hit
    groups.emplace_back(group).closestHitShader = 2;
    groups.back().type = VkRayTracingShaderGroupTypeNV::VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;

    VkRayTracingPipelineCreateInfoNV pipelineInfo = {};
    pipelineInfo.basePipelineIndex = 0;
    pipelineInfo.maxRecursionDepth = 1;
    pipelineInfo.pGroups = groups.data();
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;

    if (vk_nv_ray_tracing::vkCreateRayTracingPipelinesNV(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create ray tracing pipeline");
    } else {
        std::puts("sucessfuly created ray tracing pipeline!!");
    }
}

void RayTracedShadowsSequence::createSbtTable(VkDevice device, VmaAllocator allocator, const VkPhysicalDeviceRayTracingPropertiesNV& rtProps) {
    const uint32_t groupCount = static_cast<uint32_t>(groups.size());
    const uint32_t sbtSize = groupCount * rtProps.shaderGroupBaseAlignment;

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

    std::vector<uint8_t> shaderHandleStorage(sbtSize);

    if (vk_nv_ray_tracing::vkGetRayTracingShaderGroupHandlesNV(device, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to get rt shader group handles");
    }

    auto* data = static_cast<uint8_t*>(allocInfo.pMappedData);

    for (uint32_t group = 0; group < groupCount; group++) {
        const auto start = shaderHandleStorage.data() + group * rtProps.shaderGroupHandleSize;
        const auto end = start + rtProps.shaderGroupHandleSize;
        std::copy(start, end, data);
        data += rtProps.shaderGroupBaseAlignment;
    }
}

void RayTracedShadowsSequence::init(VkDevice device, VmaAllocator allocator, VkPhysicalDevice pdevice, VulkanShaderManager& shaderManager) {
    createPipeline(device, shaderManager);

    // get physical device memory and rtx properties
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(pdevice, &memoryProperties);

    VkPhysicalDeviceRayTracingPropertiesNV rtProperties{};
    rtProperties.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;

    VkPhysicalDeviceProperties2 pdeviceProperties2;
    pdeviceProperties2.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    pdeviceProperties2.pNext = &rtProperties;

    vkGetPhysicalDeviceProperties2(pdevice, &pdeviceProperties2);

    createSbtTable(device, allocator, rtProperties);
}

void RayTracedShadowsSequence::destroy(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool) {
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    
    vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);

    depthTexture.destroy(device);
    shadowsTexture.destroy(device);

    vmaDestroyBuffer(allocator, sbtBuffer, sbtAlloc);
}

void RayTracedShadowsSequence::execute(VkDevice device, VkCommandBuffer cmdBuffer, uint32_t width, uint32_t height, const VkPhysicalDeviceRayTracingPropertiesNV& rtProps) {
    // acquire textures for ray tracing use
    ImageMemoryBarrier(cmdBuffer, depthTexture.image, VK_IMAGE_ASPECT_DEPTH_BIT,
        0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    ImageMemoryBarrier(cmdBuffer, shadowsTexture.image, VK_IMAGE_ASPECT_COLOR_BIT,
        0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // bind the pipeline and resources
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_RAYGEN_BIT_NV, 0, sizeof(pushData), &pushData);

    VkDeviceSize rayGenOffset = 0;  // Start at the beginning of m_sbtBuffer
    VkDeviceSize missOffset = 1u * rtProps.shaderGroupBaseAlignment;  // Jump over raygen
    VkDeviceSize missStride = rtProps.shaderGroupHandleSize;
    VkDeviceSize hitOffset = 2u * rtProps.shaderGroupBaseAlignment;
    VkDeviceSize hitStride = rtProps.shaderGroupHandleSize;

    vk_nv_ray_tracing::vkCmdTraceRaysNV(cmdBuffer,
        sbtBuffer, rayGenOffset, // raygen group 
        sbtBuffer, missOffset, missStride,
        sbtBuffer, hitOffset, hitStride,
        VK_NULL_HANDLE, 0, 0,
        width, height, 1
    );
}

} // scatter
