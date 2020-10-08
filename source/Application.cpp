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
}

void VulkanApplication::destroy()
{
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
		//drawFrame();
	}

	// vkDeviceWaitIdle(device.device);
}

} // scatter
