#include "pch.h"
#include "Application.h"
#include "Vertex.h"

namespace scatter {

void VulkanApplication::init(uint32_t width, uint32_t height) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "Scatter - Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
        app->frameBufferResized = true;
    });
    
    device.init();
    swapchain.init(window, device);
    shaderManager.init(device.device);

    const auto extent = swapchain.swapChainExtent;

    objects.emplace_back().createSphere(2.0f);

    // calculate reservation sizes
    size_t allVerticesSize = 0, allIndicesSize = 0;

    for (const auto& object : objects) {
        allVerticesSize += object.vertices.size();
        allIndicesSize += object.indices.size();
    }

    // reserve both
    std::vector<Vertex> allVertices;
    std::vector<uint16_t> allIndices;
    allVertices.reserve(allVerticesSize);
    allIndices.reserve(allIndicesSize);

    // copy data from objects to all vectors
    for (const auto& object : objects) {
        allVertices.insert(allVertices.end(), object.vertices.begin(), object.vertices.end());
        allIndices.insert(allIndices.end(), object.indices.begin(), object.indices.end());
    }

    vertexBuffer.init(device, allVertices.data(), sizeof(Vertex) * allVertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer.init(device, allIndices.data(), sizeof(uint16_t) * allIndices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    std::vector<VkGeometryNV> geometries;
    
    for (const auto& object : objects) {
        VkGeometryNV geometry{};
        VkGeometryTrianglesNV& triangle = geometry.geometry.triangles;

        geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
        triangle.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
        triangle.vertexData = vertexBuffer.getBuffer();
        triangle.vertexOffset = object.vertexOffset;
        triangle.vertexCount = static_cast<uint32_t>(object.vertices.size());
        triangle.vertexStride = sizeof(Vertex);
        triangle.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangle.indexData = indexBuffer.getBuffer();
        triangle.indexOffset = 0;
        triangle.indexCount = static_cast<uint32_t>(object.indices.size());
        triangle.indexType = VK_INDEX_TYPE_UINT16;
        triangle.transformData = VK_NULL_HANDLE;
        triangle.transformOffset = 0;
        geometry.geometry.aabbs = {};
        geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

        geometries.push_back(geometry);
    }

    // create bottom level acceleration structure 
    VkAccelerationStructureCreateInfoNV BLAScreateInfo{};
    BLAScreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    BLAScreateInfo.info.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    BLAScreateInfo.info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    BLAScreateInfo.info.instanceCount = 0;
    BLAScreateInfo.info.geometryCount = static_cast<uint32_t>(geometries.size());
    BLAScreateInfo.info.pGeometries = geometries.data();

    bottomLevelAS.init(device.device, device.allocator, &BLAScreateInfo);
    bottomLevelAS.record(device, &BLAScreateInfo);

    // create top level acceleration structure
    VkAccelerationStructureCreateInfoNV TLAScreateInfo{};
    TLAScreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    TLAScreateInfo.info.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    TLAScreateInfo.info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    TLAScreateInfo.info.instanceCount = 1;

    VkAccelerationStructureInstanceNV instance;

    vk_nv_ray_tracing::vkGetAccelerationStructureHandleNV(
        device.device, 
        bottomLevelAS.as, 
        sizeof(uint64_t), 
        &instance.accelerationStructureReference);

    instance.mask = 0xff;
    instance.instanceCustomIndex = 0;
    instance.instanceShaderBindingTableRecordOffset = 0;
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;

    auto transform = glm::mat4(1.0f);
    std::memcpy(&instance.transform, glm::value_ptr(transform), sizeof(VkTransformMatrixKHR));

    topLevelAS.init(device.device, device.allocator, &TLAScreateInfo);
    topLevelAS.record(device, &instance, &TLAScreateInfo);

    // set uniform data
    renderSequence.uniforms.projection = glm::perspectiveRH(glm::radians(75.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    renderSequence.uniforms.view = glm::lookAtRH(glm::vec3(2, 4, -5), glm::vec3(0, 0, 0), { 0, 1, 0 });
    shadowSequence.pushData.inverseViewProjection = glm::inverse(renderSequence.uniforms.projection * renderSequence.uniforms.view);
    
    // init both render sequences
    shadowSequence.init(device.device, device.allocator, device.physicalDevice, shaderManager, extent);
    renderSequence.init(device.device, device.allocator, device.descriptorPool, swapchain, shaderManager, shadowSequence.depthTexture.view);

    // setup descriptor sets
    renderSequence.createDescriptorSets(device.device, device.allocator, device.descriptorPool);
    renderSequence.updateDescriptorSet(device.device, device.allocator);
    shadowSequence.createDescriptorSets(device.device, device.allocator, device.descriptorPool, topLevelAS.as);

    // allocate command buffers
    device.commandBuffers.resize(renderSequence.getFramebuffersCount());
    device.createCommandBuffers();

    // record command buffers
    for (size_t i = 0; i < device.commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(device.commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to record begin command buffer \n");
        }

        const auto extent = swapchain.swapChainExtent;

        VkPhysicalDeviceRayTracingPropertiesNV rtProps{};
        rtProps.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;

        VkPhysicalDeviceProperties2 pdProps{};
        pdProps.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        pdProps.pNext = &rtProps;

        vkGetPhysicalDeviceProperties2(device.physicalDevice, &pdProps);

        renderSequence.execute(device.device, device.commandBuffers[i], device.allocator, extent, vertexBuffer.getBuffer(), indexBuffer.getBuffer(), objects, i);
        shadowSequence.execute(device.device, device.commandBuffers[i], extent.width, extent.height, rtProps);

        if (vkEndCommandBuffer(device.commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer! \n");
        }
    }

    createSyncObjects();
}

void VulkanApplication::destroy() {
    vertexBuffer.destroy(device);
    indexBuffer.destroy(device);
    bottomLevelAS.destroy(device.device, device.allocator);
    topLevelAS.destroy(device.device, device.allocator);

    for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
        vkDestroySemaphore(device.device, imageAvailableSemaphore[i], nullptr);
        vkDestroySemaphore(device.device, renderFinishedSemaphore[i], nullptr);
        vkDestroyFence(device.device, inFlightFences[i], nullptr);
    }

    shaderManager.destroy();

    shadowSequence.destroy(device.device, device.allocator, device.descriptorPool);

    swapchain.destroy(device.instance, device.device);

    renderSequence.destroy(device.device, device.allocator);

    glfwDestroyWindow(window);
    glfwTerminate();
}

void VulkanApplication::update(float dt) {
    dt = 0;
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        auto t1 = std::chrono::high_resolution_clock::now();
        
        drawFrame();

        bool input = false;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            input = true;

        }

        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            input = true;

        }


        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            input = true;
        }

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            input = true;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            input = true;
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            input = true;
        }


        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            input = true;
        }


        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            input = true;
        }

        dt = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - t1).count() / 1000;
        std::string title = "Scatter - " + std::to_string(int(1.0f / dt)) + " fps";
        glfwSetWindowTitle(window, title.c_str());
    }

    vkDeviceWaitIdle(device.device);
}

void VulkanApplication::drawFrame() {
    vkWaitForFences(device.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device.device, swapchain.swapChain, UINT64_MAX, imageAvailableSemaphore[currentFrame],
        VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image \n");
    }


    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device.device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[currentFrame] = inFlightFences[currentFrame];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphore[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    VkCommandBuffer cmdBuffers[] = { device.commandBuffers[imageIndex] };

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = cmdBuffers;

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphore[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device.device, 1, &inFlightFences[currentFrame]);

    if (auto submit = vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]); submit != VK_SUCCESS) {
        std::cout << submit << std::endl;
        throw std::runtime_error("failed to submit draw command buffer! \n");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapchain.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(swapchain.presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frameBufferResized) {
        frameBufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image! \n");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAME_IN_FLIGHT;
}

void VulkanApplication::recreateSwapChain() {
    int width = 0;
    int height = 0;

    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 && height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device.device);

    swapchain.destroy(device.instance, device.device);
    renderSequence.destroyFramebuffers(device.device);
    shadowSequence.destroyImages(device.device);

    swapchain.init(window, device);
    
    // create the depth and shadow textures
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &memoryProperties);
    shadowSequence.createImages(device.device, swapchain.swapChainExtent, &memoryProperties);
    shadowSequence.updateImages(device.device);
    
    renderSequence.createFramebuffers(device.device, swapchain.swapChainImageViews, swapchain.swapChainExtent, shadowSequence.depthTexture.view);
 
    // re-allocate command buffers
    device.commandBuffers.resize(renderSequence.getFramebuffersCount());
    device.createCommandBuffers();

    VkPhysicalDeviceRayTracingPropertiesNV rtProps{};
    rtProps.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;

    VkPhysicalDeviceProperties2 pdProps{};
    pdProps.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    pdProps.pNext = &rtProps;

    vkGetPhysicalDeviceProperties2(device.physicalDevice, &pdProps);

    // record command buffers
    for (size_t i = 0; i < device.commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(device.commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to record begin command buffer \n");
        }

        const auto extent = swapchain.swapChainExtent;

        renderSequence.execute(device.device, device.commandBuffers[i], device.allocator, extent, vertexBuffer.getBuffer(), indexBuffer.getBuffer(), objects, i);
        shadowSequence.execute(device.device, device.commandBuffers[i], extent.width, extent.height, rtProps);

        if (vkEndCommandBuffer(device.commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer! \n");
        }
    }

}

void VulkanApplication::createSyncObjects() {
    imageAvailableSemaphore.resize(MAX_FRAME_IN_FLIGHT);
    renderFinishedSemaphore.resize(MAX_FRAME_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAME_IN_FLIGHT);
    imagesInFlight.resize(swapchain.swapChainImageViews.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore[i]) != VK_SUCCESS ||
            vkCreateFence(device.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores! \n");
        } else {
            std::cout << "successfully created semaphores! \n";
        }
    }
}

} // scatter
