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

    renderSequence.uniforms.projection = glm::perspectiveRH(glm::radians(75.0f), 16.0f / 9.0f, 0.1f, 10000.0f);
    renderSequence.uniforms.view = glm::lookAtRH(glm::vec3(2, 4, -5), glm::vec3(0, 0, 0), { 0, 1, 0 });

    renderSequence.init(device.device, device.allocator, device.descriptorPool, swapchain, shaderManager);

    auto& obj = objects.emplace_back();

    const float radius = 2.0f;
    float x, y, z, xy;                              // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
    float s, t;                                     // vertex texCoord

    const int sectorCount = 36;
    const int stackCount = 18;
    const float PI = static_cast<float>(3.14);
    float sectorStep = 2 * PI / sectorCount;
    float stackStep = PI / stackCount;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= stackCount; ++i) {
        stackAngle = PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);             // r * cos(u)
        z = radius * sinf(stackAngle);              // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for (int j = 0; j <= sectorCount; ++j) {
            Vertex v;

            sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
            v.pos = glm::vec3(x, y, z);

            // normalized vertex normal (nx, ny, nz)
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            v.normal = glm::vec3(nx, ny, nz);

            // vertex tex coord (s, t) range between [0, 1]
            s = (float)j / sectorCount;
            t = (float)i / stackCount;
            v.texcoord = glm::vec2(s, t);

            obj.vertices.push_back(v);
        }
    }

    int k1, k2;
    for (int i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1);     // beginning of current stack
        k2 = k1 + sectorCount + 1;      // beginning of next stack

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            if (i != 0) {

                obj.indices.push_back(k1);
                obj.indices.push_back(k2);
                obj.indices.push_back(k1 + 1);
            }

            // k1+1 => k2 => k2+1
            if (i != (stackCount - 1)) {
                obj.indices.push_back(k1 + 1);
                obj.indices.push_back(k2);
                obj.indices.push_back(k2 + 1);
            }
        }
    }

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

    device.commandBuffers.resize(renderSequence.getFramebuffersCount());
    device.createCommandBuffers();

    for (size_t i = 0; i < device.commandBuffers.size(); i++) {
        renderSequence.recordCommandBuffer(device.device, device.commandBuffers[i], device.allocator, swapchain.swapChainExtent, vertexBuffer.getBuffer(), indexBuffer.getBuffer(), objects, i);
    }

    createSyncObjects();
}

void VulkanApplication::destroy() {
    vertexBuffer.destroy(device);
    indexBuffer.destroy(device);

    for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
        vkDestroySemaphore(device.device, imageAvailableSemaphore[i], nullptr);
        vkDestroySemaphore(device.device, renderFinishedSemaphore[i], nullptr);
        vkDestroyFence(device.device, inFlightFences[i], nullptr);
    }

    shaderManager.destroy();

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
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &device.commandBuffers[imageIndex];
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphore[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device.device, 1, &inFlightFences[currentFrame]);

    if (vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
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
    renderSequence.destroyDepthTexture(device.device, device.allocator);
    renderSequence.destroyFramebuffers(device.device);

    swapchain.init(window, device);
    renderSequence.createDepthTexture(device.device, device.allocator, swapchain);
    renderSequence.createFramebuffers(device.device, swapchain.swapChainImageViews, swapchain.swapChainExtent);
 
    device.commandBuffers.resize(renderSequence.getFramebuffersCount());
    device.createCommandBuffers();
    for (size_t i = 0; i < device.commandBuffers.size(); i++) {
        renderSequence.recordCommandBuffer(device.device, device.commandBuffers[i], device.allocator, swapchain.swapChainExtent, vertexBuffer.getBuffer(), indexBuffer.getBuffer(), objects, i);
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
