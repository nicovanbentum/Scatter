#pragma once

#ifndef VK_LOAD_FN
#define VK_LOAD_FN(device, x) PFN_##x(vkGetDeviceProcAddr(device, #x))
#endif

#include "vulkan/vulkan.h"

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
};

}
