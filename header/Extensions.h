#pragma once

#include "pch.h"

#ifndef VK_LOAD_FN
#define VK_LOAD_FN(device, x) PFN_##x(vkGetDeviceProcAddr(device, #x))
#endif

namespace scatter {

class Extension {
public:
    virtual const char* const* getDeviceExtensionNames(uint32_t* count) = 0;
    virtual const char* const* getInstanceExtensionNames(uint32_t* count) = 0;
};

class RayTracingNV  : public Extension {
public:
    // function pointers
    PFN_vkCreateAccelerationStructureNV                  vkCreateAccelerationStructureNV;
    PFN_vkDestroyAccelerationStructureNV                 vkDestroyAccelerationStructureNV;
    PFN_vkBindAccelerationStructureMemoryNV              vkBindAccelerationStructureMemoryNV;
    PFN_vkGetAccelerationStructureHandleNV               vkGetAccelerationStructureHandleNV;
    PFN_vkGetAccelerationStructureMemoryRequirementsNV   vkGetAccelerationStructureMemoryRequirementsNV;
    PFN_vkCmdBuildAccelerationStructureNV                vkCmdBuildAccelerationStructureNV;
    PFN_vkCreateRayTracingPipelinesNV                    vkCreateRayTracingPipelinesNV;
    PFN_vkGetRayTracingShaderGroupHandlesNV              vkGetRayTracingShaderGroupHandlesNV;
    PFN_vkCmdTraceRaysNV                                 vkCmdTraceRaysNV;

    RayTracingNV() = default;
    void init(VkDevice device) {
        vkCreateAccelerationStructureNV = VK_LOAD_FN(device, vkCreateAccelerationStructureNV);
        vkDestroyAccelerationStructureNV = VK_LOAD_FN(device, vkDestroyAccelerationStructureNV);
        vkBindAccelerationStructureMemoryNV = VK_LOAD_FN(device, vkBindAccelerationStructureMemoryNV);
        vkGetAccelerationStructureHandleNV = VK_LOAD_FN(device, vkGetAccelerationStructureHandleNV);
        vkGetAccelerationStructureMemoryRequirementsNV = VK_LOAD_FN(device, vkGetAccelerationStructureMemoryRequirementsNV);
        vkCmdBuildAccelerationStructureNV = VK_LOAD_FN(device, vkCmdBuildAccelerationStructureNV);
        vkCreateRayTracingPipelinesNV = VK_LOAD_FN(device, vkCreateRayTracingPipelinesNV);
        vkGetRayTracingShaderGroupHandlesNV = VK_LOAD_FN(device, vkGetRayTracingShaderGroupHandlesNV);
        vkCmdTraceRaysNV = VK_LOAD_FN(device, vkCmdTraceRaysNV);
    }


    virtual const char* const * getDeviceExtensionNames(uint32_t* count) {
        *count = static_cast<uint32_t>(deviceExtensionNames.size());
        return deviceExtensionNames.data();

    }
    virtual const char* const* getInstanceExtensionNames(uint32_t* count) {
        *count = static_cast<uint32_t>(1);
        return &instanceExtensionNames;
    }

    const std::array<const char*, 2> deviceExtensionNames {
        VK_NV_RAY_TRACING_EXTENSION_NAME, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
    };

    const char* instanceExtensionNames = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
};

}
