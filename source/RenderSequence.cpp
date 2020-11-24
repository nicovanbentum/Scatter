#include "pch.h"
#include "RenderSequence.h"
#include "Swapchain.h"
#include "Vertex.h"
#include "Object.h"



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

void VulkanRenderSequence::init(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager, VkImageView depthView) {
    createRenderPass(device, swapchain);
    createGraphicsPipeline(device, descriptorPool, swapchain, shaderManager);
    createFramebuffers(device, swapchain.swapChainImageViews, swapchain.swapChainExtent, depthView);
}

void VulkanRenderSequence::destroyFramebuffers(VkDevice device) {
    for (auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
}

void VulkanRenderSequence::destroy(VkDevice device, VmaAllocator allocator) {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    vmaDestroyBuffer(allocator, uniformBuffer, uniformBufferAlloc);
}

void VulkanRenderSequence::createRenderPass(VkDevice device, const VulkanSwapchain& swapchain) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchain.swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass! \n");
    }
}

void VulkanRenderSequence::createGraphicsPipeline(VkDevice device, VkDescriptorPool descriptorPool, const VulkanSwapchain& swapchain, VulkanShaderManager& shaderManager) {
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = shaderManager.getShader("shader/vert.spv");
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = shaderManager.getShader("shader/frag.spv");
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain.swapChainExtent.width);
    viewport.height = static_cast<float>(swapchain.swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain.swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkDescriptorSetLayoutBinding uniformBufferBinding = {};
    uniformBufferBinding.binding = 0;
    uniformBufferBinding.descriptorCount = 1;
    uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferBinding.pImmutableSamplers = nullptr;
    uniformBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.bindingCount = 1;
    descriptorSetLayoutInfo.pBindings = &uniformBufferBinding;

    if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout");
    } else {
        std::puts("Created descriptor set layout");
    }

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout! \n");
    } else {
        std::cout << "successfully created pipeline layout! \n";
    }

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline! \n");
    } else {
        std::cout << "successfully created graphics pipeline! \n";
    }
}

void VulkanRenderSequence::createFramebuffers(VkDevice device, const std::vector<VkImageView>& imageViews, VkExtent2D extent, VkImageView depthView) {
    framebuffers.resize(imageViews.size());
    for (size_t i = 0; i < imageViews.size(); i++) {

        std::array<VkImageView, 2> attachments = { imageViews[i], depthView };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer! \n");
        } else {
            std::cout << "successfully created framebuffer " << i << " ! \n";
        }
    }
}

void VulkanRenderSequence::createDescriptorSets(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool) {
    // setup the uniform buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(Uniforms);
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &uniformBuffer, &uniformBufferAlloc, &uniformBufferAllocInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate uniform buffer");
    } else {
        std::puts("Succesfully created Uniform Buffer!!");
    }

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

    // setup the write descriptor set for updating 
    descriptorBufferInfo = {};
    descriptorBufferInfo.buffer = uniformBuffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = VK_WHOLE_SIZE;

    writeDescriptorSet = {};
    writeDescriptorSet.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.dstSet = descriptorSet;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

    memcpy(uniformBufferAllocInfo.pMappedData, &uniforms, uniformBufferAllocInfo.size);
    vmaFlushAllocation(allocator, uniformBufferAlloc, 0, uniformBufferAllocInfo.size);
    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
}

void VulkanRenderSequence::updateDescriptorSet(VkDevice device, VmaAllocator allocator) {
    memcpy(uniformBufferAllocInfo.pMappedData, &uniforms, uniformBufferAllocInfo.size);
    vmaFlushAllocation(allocator, uniformBufferAlloc, 0, uniformBufferAllocInfo.size);
}

void VulkanRenderSequence::execute(VkDevice device, VkCommandBuffer commandBuffer, VmaAllocator allocator, VkExtent2D extent, VkBuffer vertexBuffer, VkBuffer indexBuffer, const std::vector<Object>& objects, size_t framebufferIndex) {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[framebufferIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent;


    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offset[] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offset);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    for (const auto& object : objects) {
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &object.model);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object.indices.size()), 1, object.indexOffset, object.vertexOffset, 0);
    }
    
    vkCmdEndRenderPass(commandBuffer);
}

HANDLE RayTracedShadowsSequence::getMemoryHandle(VkDevice device, VkDeviceMemory memory) {
    auto vkGetMemoryWin32HandleKHR = PFN_vkGetMemoryWin32HandleKHR(vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR"));

    VkMemoryGetWin32HandleInfoKHR handleInfo = {};
    handleInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    handleInfo.handleType = VkExternalMemoryHandleTypeFlagBits::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    handleInfo.memory = memory;

    HANDLE handle;
    vkGetMemoryWin32HandleKHR(device, &handleInfo, &handle);
    return handle;
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

void RayTracedShadowsSequence::createDescriptorSets(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool, VkAccelerationStructureNV tlas) {
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

    updateImages(device);
    vkUpdateDescriptorSets(device, 1u, &writeAS, 0, nullptr);
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

    std::cout << "base alignment " << rtProps.shaderGroupBaseAlignment << std::endl;
    std::cout << "group handle size " << rtProps.shaderGroupHandleSize << std::endl;

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

void RayTracedShadowsSequence::init(VkDevice device, VmaAllocator allocator, VkPhysicalDevice pdevice, VulkanShaderManager& shaderManager, VkExtent2D extent) {
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

    createImages(device, extent, &memoryProperties);
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
