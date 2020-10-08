#include "pch.h"
#include "Application.h"

namespace scatter {

void VulkanApplication::init(uint32_t width, uint32_t height)
{
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
	
	renderSequence.init(device.device, swapchain, shaderManager);
	
	const std::vector<Vertex> vertices = {
	{{0.0f, -0.1f}, {0.0f, 0.0f, 1.0f}},
	{{0.3f, 0.3f}, {0.0f, 0.0f, 1.0f}},
	{{-0.3f, 0.3f}, {0.0f, 0.0f, 1.0f}}
	};

	commandBufferManager.init(device);
	vertexBuffer.init(device, commandBufferManager, vertices);
	commandBufferManager.recordCommandBuffer(device.device, renderSequence, swapchain.swapChainExtent, vertexBuffer);
	createSyncObjects();
}

void VulkanApplication::destroy()
{
	vertexBuffer.destroy(device);
	commandBufferManager.destroy(device.device);
	for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device.device, imageAvailableSemaphore[i], nullptr);
		vkDestroySemaphore(device.device, renderFinishedSemaphore[i], nullptr);
		vkDestroyFence(device.device, inFlightFences[i], nullptr);
	}

	shaderManager.destroy();

	swapchain.destroy(device.device);

	renderSequence.destroy(device.device);

	glfwDestroyWindow(window);
	glfwTerminate();
}

void VulkanApplication::update(float dt)
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		drawFrame();
	}

	 vkDeviceWaitIdle(device.device);
}

void VulkanApplication::drawFrame()
{
	vkWaitForFences(device.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device.device, swapchain.swapChain, UINT64_MAX, imageAvailableSemaphore[currentFrame],
		VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image \n");
	}


	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
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
	submitInfo.pCommandBuffers = &commandBufferManager.commandBuffers[imageIndex];
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device.device, 1, &inFlightFences[currentFrame]);

	if (vkQueueSubmit( device.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
	{
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

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frameBufferResized)
	{
		frameBufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image! \n");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAME_IN_FLIGHT;
}

void VulkanApplication::recreateSwapChain() {
	int width = 0;
	int height = 0;

	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 && height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device.device);

	swapchain.destroy(device.device);
	renderSequence.destroyFramebuffers(device.device);

	swapchain.init(window, device);
	renderSequence.createFramebuffers(device.device, swapchain.swapChainImageViews, swapchain.swapChainExtent);
	commandBufferManager.recordCommandBuffer(device.device, renderSequence, swapchain.swapChainExtent, vertexBuffer);
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

	for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore[i]) != VK_SUCCESS ||
			vkCreateFence(device.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores! \n");
		}
		else
		{
			std::cout << "successfully created semaphores! \n";
		}
	}
}

} // scatter
