//#include "pch.h"
//#include "CommandBufferManager.h"
//#include "Objects.h"
//
//namespace scatter {
//
//void CommandBufferManager::init(VulkanDevice& vulkanDevice) {
//    createCommandPool(vulkanDevice);
//}
//
//void CommandBufferManager::destroy(VkDevice device) {
//    vkDestroyCommandPool(device, commandPool, nullptr);
//}
//
//VkCommandBuffer CommandBufferManager::beginSingleTimeCommands(VulkanDevice& device) {
//    VkCommandBufferAllocateInfo allocInfo = {};
//    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//    allocInfo.commandPool = commandPool;
//    allocInfo.commandBufferCount = 1;
//
//    VkCommandBuffer commandBuffer;
//    vkAllocateCommandBuffers(device.device, &allocInfo, &commandBuffer);
//
//    VkCommandBufferBeginInfo beginInfo = {};
//    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//
//    vkBeginCommandBuffer(commandBuffer, &beginInfo);
//
//    return commandBuffer;
//}
//
//void CommandBufferManager::endSingleTimeCommands(VulkanDevice& device, VkCommandBuffer buffer) {
//    vkEndCommandBuffer(buffer);
//
//    VkSubmitInfo submitInfo = {};
//    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//    submitInfo.commandBufferCount = 1;
//    submitInfo.pCommandBuffers = &buffer;
//
//    if (vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
//        throw std::runtime_error("failed to submit single time queue");
//    }
//
//    vkQueueWaitIdle(device.graphicsQueue);
//
//    vkFreeCommandBuffers(device.device, commandPool, 1, &buffer);
//}
//
//void CommandBufferManager::createCommandPool(VulkanDevice& vulkanDevice) {
//    QueueFamilyIndices queueFamilyIndices = vulkanDevice.findQueueFamilies(vulkanDevice.physicalDevice);
//
//    VkCommandPoolCreateInfo poolInfo{};
//    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
//    poolInfo.flags = 0;
//
//    if (vkCreateCommandPool(vulkanDevice.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create command pool! \n");
//    } else {
//        std::cout << "successfully created command pool! \n";
//    }
//}
//
//void CommandBufferManager::recordCommandBuffer(const VkDevice device, VulkanRenderSequence& renderSequence, VkExtent2D extent, Object& object) {
//    VkViewport viewport{};
//    viewport.x = 0.0f;
//    viewport.y = 0.0f;
//    viewport.width = static_cast<float>(extent.width);
//    viewport.height = static_cast<float>(extent.height);
//    viewport.minDepth = 0.0f;
//    viewport.maxDepth = 1.0f;
//
//    VkRect2D scissor{};
//    scissor.offset = { 0, 0 };
//    scissor.extent = extent;
//
//    commandBuffers.resize(renderSequence.framebuffers.size());
//    VkCommandBufferAllocateInfo allocInfo{};
//    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//    allocInfo.commandPool = commandPool;
//    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
//
//    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
//        throw std::runtime_error("failed to allocate command buffers! \n");
//    } else {
//        std::cout << "successfully allocated command buffers! \n";
//    }
//
//    for (size_t i = 0; i < commandBuffers.size(); i++) {
//        VkCommandBufferBeginInfo beginInfo{};
//        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//        beginInfo.flags = 0;
//        beginInfo.pInheritanceInfo = nullptr;
//
//        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
//            throw std::runtime_error("failed to record begin command buffer \n");
//        } else {
//            std::cout << "successfully recorder begin command buffer! \n";
//        }
//
//        VkRenderPassBeginInfo renderPassInfo{};
//        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//        renderPassInfo.renderPass = renderSequence.renderPass;
//        renderPassInfo.framebuffer = renderSequence.framebuffers[i];
//        renderPassInfo.renderArea.offset = { 0, 0 };
//        renderPassInfo.renderArea.extent = extent;
//        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
//        renderPassInfo.clearValueCount = 1;
//        renderPassInfo.pClearValues = &clearColor;
//
//        vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);
//        vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);
//        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
//        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderSequence.pipeline);
//
//        VkBuffer vertexBuffers[] = { object.vertexBuffer.buffer };
//        VkDeviceSize offset[] = { 0 };
//        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offset);
//        vkCmdBindIndexBuffer(commandBuffers[i], object.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
//        vkCmdDrawIndexed(commandBuffers[i], object.indices.size(), 1, 0, 0, 0);
//
//        vkCmdEndRenderPass(commandBuffers[i]);
//        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
//            throw std::runtime_error("failed to record command buffer! \n");
//        } else {
//            std::cout << "successfully recorded command buffer! \n";
//        }
//    }
//}
//
//} // scatter
