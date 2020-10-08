#include "pch.h"
#include "ShaderManager.h"

namespace scatter {

std::vector<char> VulkanShaderManager::readFromSpirv(const std::filesystem::path& pathName)
{
	std::ifstream file(pathName, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		std::cout << pathName << std::endl;
		throw std::runtime_error("failed to open file! \n");
	}
	else
	{
		std::cout << "file " << pathName << " succesfully opened! \n";
	}

	size_t fileSize = (size_t)file.tellg();
	std::cout << pathName << " is " << fileSize << " large. \n";
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

VkShaderModule VulkanShaderManager::addShader(const std::filesystem::path& path)
{
	auto code = readFromSpirv(path);

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module! \n");
	}
	else
	{
		std::cout << "successfully created shader module! \n";
	}

	shaders[path.string()] = shaderModule;

	return shaderModule;
}

void VulkanShaderManager::init(VkDevice device)
{
	this->device = device;
}

void VulkanShaderManager::destroy()
{
	for (auto& shader : shaders) {
		vkDestroyShaderModule(device, shader.second, nullptr);
	}
}

VkShaderModule VulkanShaderManager::getShader(const std::filesystem::path& path)
{
	return shaders[path.string()];
}

bool VulkanShaderManager::compile(const std::string& filenameIn, const std::string& filenameOut)
{
	const auto vulkan_sdk_path = getenv("VULKAN_SDK");
	if (!vulkan_sdk_path) {
		std::puts("Unable to find Vulkan SDK, cannot compile shader");
		return false;
	}

	const auto compiler = vulkan_sdk_path + std::string("\\Bin\\glslc.exe ");
	const auto command = compiler + std::string(filenameIn) + " -o " + std::string(filenameOut);

	if (system(command.c_str()) != 0) {
		std::cout << "failed to compile vulkan shader: " << filenameIn << '\n';
	}
	else {
		std::cout << "Successfully compiled VK shader: " << filenameIn << '\n';
	}

	return true;
}

} // scatter
