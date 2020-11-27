#pragma once

#pragma once

#include "Extensions.h"

namespace scatter {

struct DeviceQueue {
    VkQueue   handle = VK_NULL_HANDLE;
    uint32_t  family = 0;
    uint32_t  index = 0;
};

struct DeviceQueueSet {
    DeviceQueue graphics;
    DeviceQueue transfer;
};

struct AdapterQueueIndices {
    uint32_t graphics;
    uint32_t transfer;
};

class Instance {
public:
    Instance();
    ~Instance();

    VkInstance handle();
    std::vector<VkPhysicalDevice> getAdapters();

private:
    VkInstance instance;
};

class Adapter {
public:
    Adapter(const std::shared_ptr<Instance>& instance);
    ~Adapter();

    VkPhysicalDevice handle();
    AdapterQueueIndices getQueueIndices();
    uint32_t findQueueFamily(VkQueueFlags mask, VkQueueFlags flags) const;


private:
    VkPhysicalDevice m_adapter = VK_NULL_HANDLE;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties properties;
    std::vector<VkQueueFamilyProperties> queueFamilies;
};

class Device {
    Device(const std::shared_ptr<Instance>& instance,
            const std::shared_ptr<Adapter>& adapter);

    std::shared_ptr<Adapter> adapter;
    std::shared_ptr<Instance> instance;

    VkDevice device;
    VmaAllocator allocator;
    DeviceQueueSet queueSet;
};

} // scatter
