#include "pch.h"
#include "NewDevice.h"

namespace scatter {

Instance::Instance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Scatter Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
    appInfo.pEngineName = "Scatter";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    const char* validationLayer = "VK_LAYER_KHRONOS_validation";

    if (IS_DEBUG) {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &validationLayer;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    //// synchronization validation layers, pls save me
    //if (IS_DEBUG) {
    //    std::vector<VkValidationFeatureEnableEXT> enables = {
    //        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    //    };
    //    VkValidationFeaturesEXT features = {};
    //    features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    //    features.enabledValidationFeatureCount = static_cast<uint32_t>(enables.size());
    //    features.pEnabledValidationFeatures = enables.data();
    //    createInfo.pNext = &features;
    //}

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance! \n");
    } else {
        std::cout << "created instance succesfully \n";
    }
}

Instance::~Instance() { vkDestroyInstance(instance, nullptr); }

std::vector<VkPhysicalDevice> Instance::getAdapters() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> adapters(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, adapters.data());
    return adapters;
}

VkInstance Instance::handle() { return instance; }

Adapter::Adapter(const std::shared_ptr<Instance>& instance) {
    auto adapters = instance->getAdapters();
    if (adapters.empty()) {
        throw std::runtime_error("no adapters found");
    }

    m_adapter = adapters[0];

    // TODO: check for ray tracing support
    for (const auto& adapter : adapters) {
        vkGetPhysicalDeviceProperties(adapter, &properties);

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            m_adapter = adapter;
            break;
        }
    }

    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_adapter, &familyCount, nullptr);
    vkGetPhysicalDeviceQueueFamilyProperties(m_adapter, &familyCount, queueFamilies.data());
}

Adapter::~Adapter() {}

uint32_t Adapter::findQueueFamily(VkQueueFlags mask, VkQueueFlags flags) const {
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if ((queueFamilies[i].queueFlags & mask) == flags)
            return i;
    }

    return VK_QUEUE_FAMILY_IGNORED;
}

AdapterQueueIndices Adapter::getQueueIndices() {
    uint32_t graphicsQueue = findQueueFamily(
        VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT,
        VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

    uint32_t computeQueue = findQueueFamily(
        VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT,
        VK_QUEUE_COMPUTE_BIT);

    if (computeQueue == VK_QUEUE_FAMILY_IGNORED)
        computeQueue = graphicsQueue;

    uint32_t transferQueue = findQueueFamily(
        VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT,
        VK_QUEUE_TRANSFER_BIT);

    if (transferQueue == VK_QUEUE_FAMILY_IGNORED)
        transferQueue = computeQueue;

    AdapterQueueIndices queues;
    queues.graphics = graphicsQueue;
    queues.transfer = transferQueue;
    return queues;
}

VkPhysicalDevice Adapter::handle() { return m_adapter; }

Device::Device(const std::shared_ptr<Instance>& instance, const std::shared_ptr<Adapter>& adapter) {

    auto queueFamilyIndices = adapter->getQueueIndices();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilyIndices.graphics,
        queueFamilyIndices.transfer
    };

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndices.graphics;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{};

    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_NV_RAY_TRACING_EXTENSION_NAME, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME };

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    if (IS_DEBUG) {
        const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    if (vkCreateDevice(adapter->handle(), &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device! \n");
    } else {
        std::cout << "Created logical device! \n";
    }

    vkGetDeviceQueue(device, queueFamilyIndices.graphics, 0, &queueSet.graphics.handle);
    vkGetDeviceQueue(device, queueFamilyIndices.transfer, 0, &queueSet.transfer.handle);
}

} // scatter

