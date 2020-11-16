#include "pch.h"
#include "Application.h"
#include "Vertex.h"

namespace scatter {

class Camera {
    glm::mat4 getViewMatrix() {
        return glm::lookAtRH(position, position + getDirection(), { 0, 1, 0 });
    }

    glm::vec3 getDirection() {
        return glm::vec3(cos(angle.y) * sin(angle.x),
            sin(angle.y), cos(angle.y) * cos(angle.x));
    }

    glm::mat4 getProjectionMatrix() {
        glm::perspectiveRH(glm::radians(75.0f), 16.0f / 9.0f, 0.1f, 10000.0f);
    }

    void strafe() {

    }
    
private:
    glm::vec2 angle;
    glm::vec3 position;
};

void VulkanApplication::init(uint32_t width, uint32_t height) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);

    auto framebufferResizeCallback = [](GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
        app->frameBufferResized = true;
    };
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    device.init(window);
    swapchain.init(window, device);

    shaderManager.init(device.device);
    shaderManager.addShader("shader/vert.spv");
    shaderManager.addShader("shader/frag.spv");
    shaderManager.addShader("shader/raytrace.rgen.spv");
    shaderManager.addShader("shader/raytrace.rmiss.spv");

    auto& obj = objects.emplace_back();
    obj.createSphere();

    obj.vertexOffset = 0;
    obj.indexOffset = 0;

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
        triangle.vertexOffset = obj.vertexOffset;
        triangle.vertexCount = static_cast<uint32_t>(obj.vertices.size());
        triangle.vertexStride = sizeof(Vertex);
        triangle.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangle.indexData = indexBuffer.getBuffer();
        triangle.indexOffset = 0;
        triangle.indexCount = static_cast<uint32_t>(obj.indices.size());
        triangle.indexType = VK_INDEX_TYPE_UINT16;
        triangle.transformData = VK_NULL_HANDLE;
        triangle.transformOffset = 0;
        geometry.geometry.aabbs = {};
        geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

        geometries.push_back(geometry);
    }

    // create scene acceleration structure for ray tracing 
    VkAccelerationStructureCreateInfoNV BLAScreateInfo{};
    BLAScreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    BLAScreateInfo.info.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    BLAScreateInfo.info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    BLAScreateInfo.info.instanceCount = 0;
    BLAScreateInfo.info.geometryCount = static_cast<uint32_t>(geometries.size());
    BLAScreateInfo.info.pGeometries = geometries.data();

    bottomLevelAS.init(device.device, device.allocator, &BLAScreateInfo);
    bottomLevelAS.record(device, &BLAScreateInfo);

    VkAccelerationStructureCreateInfoNV TLAScreateInfo{};
    TLAScreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    TLAScreateInfo.info.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    TLAScreateInfo.info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    TLAScreateInfo.info.instanceCount = 1;

    Instance instance;
    instance.BLAS = bottomLevelAS.as;
    instance.hitGroupIndex = 0;
    instance.instanceID = 0;
    instance.transform = glm::mat4(1.0f);

    topLevelAS.init(device.device, device.allocator, &TLAScreateInfo);
    topLevelAS.record(device, &instance, &TLAScreateInfo);


    shadowSequence.createPipeline(device.device, device.descriptorPool, swapchain, shaderManager);
    
    // create the depth and shadow textures
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &memoryProperties);
    shadowSequence.createImages(device.device, swapchain.swapChainExtent, &memoryProperties);

    renderSequence.uniforms.projection = glm::perspectiveRH(glm::radians(75.0f), 16.0f / 9.0f, 0.1f, 10000.0f);
    renderSequence.uniforms.view = glm::lookAtRH(glm::vec3(2, 4, -5), glm::vec3(0, 0, 0), { 0, 1, 0 });
    renderSequence.init(device.device, device.allocator, device.descriptorPool, swapchain, shaderManager, shadowSequence.depthTexture.view);

    // setup descriptor sets
    shadowSequence.createDescriptorSets(device.device, device.allocator, device.descriptorPool, topLevelAS.as);

    // setup Shader Binding Table
    VkPhysicalDeviceRayTracingPropertiesNV rtProps = {
        .sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV,
    };

    VkPhysicalDeviceProperties2 pdProps = {
        .sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &rtProps
    };

    vkGetPhysicalDeviceProperties2(device.physicalDevice, &pdProps);

    shadowSequence.createSbtTable(device.device, device.allocator, rtProps);

    device.commandBuffers.resize(renderSequence.getFramebuffersCount());
    device.createCommandBuffers();

    for (size_t i = 0; i < device.commandBuffers.size(); i++) {
        renderSequence.recordCommandBuffer(device.device, device.commandBuffers[i], device.allocator, swapchain.swapChainExtent, vertexBuffer.getBuffer(), indexBuffer.getBuffer(), objects, i);
    }

    // record ray trace command buffers
    device.createRtCommandBuffers();
    shadowSequence.record(device.device, device.raytraceCommands, swapchain.swapChainExtent.width, swapchain.swapChainExtent.height, rtProps);

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

    vkDestroyFence(device.device, raytracingDoneFence, nullptr);

    shaderManager.destroy();

    shadowSequence.destroy(device.device, device.allocator, device.descriptorPool);

    swapchain.destroy(device.device);

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

        if (input) {

            device.commandBuffers.resize(renderSequence.getFramebuffersCount());
            device.createCommandBuffers();
            for (size_t i = 0; i < device.commandBuffers.size(); i++) {
                renderSequence.updateDescriptorSet(device.device, device.allocator);
                renderSequence.recordCommandBuffer(device.device, device.commandBuffers[i], device.allocator, swapchain.swapChainExtent, vertexBuffer.getBuffer(), indexBuffer.getBuffer(), objects, i);
            }
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
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
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

    result = vkQueuePresentKHR(device.presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frameBufferResized) {
        frameBufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image! \n");
    }

    VkSubmitInfo rtSubmit = {};
    rtSubmit.commandBufferCount = 1;
    rtSubmit.sType = VkStructureType::VK_STRUCTURE_TYPE_SUBMIT_INFO;
    rtSubmit.pCommandBuffers = &device.raytraceCommands;

    VkShaderStageFlags waitFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV;
    rtSubmit.pWaitDstStageMask = &waitFlags;
    rtSubmit.waitSemaphoreCount = 1;
    rtSubmit.pWaitSemaphores = signalSemaphores;

    //vkWaitForFences(device.device, 1, &raytracingDoneFence, VK_TRUE, UINT64_MAX);

    vkResetFences(device.device, 1, &raytracingDoneFence);

    //if (auto submit = vkQueueSubmit(device.graphicsQueue, 1, &rtSubmit, raytracingDoneFence); submit != VK_SUCCESS) {
    //    std::cout << submit << std::endl;
    //    throw std::runtime_error("failed to submit RT commands");
    //}

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

    swapchain.destroy(device.device);
    renderSequence.destroyFramebuffers(device.device);
    shadowSequence.destroyImages(device.device);

    swapchain.init(window, device);
    
    // create the depth and shadow textures
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &memoryProperties);
    shadowSequence.createImages(device.device, swapchain.swapChainExtent, &memoryProperties);
    shadowSequence.updateImages(device.device);
    
    renderSequence.createFramebuffers(device.device, swapchain.swapChainImageViews, swapchain.swapChainExtent, shadowSequence.depthTexture.view);
 
    device.commandBuffers.resize(renderSequence.getFramebuffersCount());
    device.createCommandBuffers();
    for (size_t i = 0; i < device.commandBuffers.size(); i++) {
        renderSequence.recordCommandBuffer(device.device, device.commandBuffers[i], device.allocator, swapchain.swapChainExtent, vertexBuffer.getBuffer(), indexBuffer.getBuffer(), objects, i);
    }

    VkPhysicalDeviceRayTracingPropertiesNV rtProps = {
        .sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV,
    };

    VkPhysicalDeviceProperties2 pdProps = {
        .sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &rtProps
    };

    vkGetPhysicalDeviceProperties2(device.physicalDevice, &pdProps);

    device.createRtCommandBuffers();
    shadowSequence.record(device.device, device.raytraceCommands, swapchain.swapChainExtent.width, swapchain.swapChainExtent.height, rtProps);

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

    vkCreateFence(device.device, &fenceInfo, nullptr, &raytracingDoneFence);
}

} // scatter
