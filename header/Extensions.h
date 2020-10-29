#pragma once

#include "pch.h"

#ifndef VK_LOAD_FN
#define VK_LOAD_FN(device, x) PFN_##x(vkGetDeviceProcAddr(device, #x))
#endif

namespace scatter {

class vk_nv_ray_tracing {
public:
    inline static PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;
    inline static PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
    inline static PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
    inline static PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
    inline static PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
    inline static PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
    inline static PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
    inline static PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
    inline static PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;

    static void init(VkDevice device);

    static constexpr std::array<const char*, 2> deviceStrings = {
        VK_NV_RAY_TRACING_EXTENSION_NAME, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
    };

    static constexpr std::array<const char*, 1> instanceStrings = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
    };
};

}
