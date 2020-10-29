#include "pch.h"
#include "Extensions.h"

namespace scatter {

void vk_nv_ray_tracing::init(VkDevice device) {
    vkCmdTraceRaysNV =                                  VK_LOAD_FN(device, vkCmdTraceRaysNV);
    vkCreateRayTracingPipelinesNV =                     VK_LOAD_FN(device, vkCreateRayTracingPipelinesNV);
    vkCreateAccelerationStructureNV =                   VK_LOAD_FN(device, vkCreateAccelerationStructureNV);
    vkDestroyAccelerationStructureNV =                  VK_LOAD_FN(device, vkDestroyAccelerationStructureNV);
    vkCmdBuildAccelerationStructureNV =                 VK_LOAD_FN(device, vkCmdBuildAccelerationStructureNV);
    vkGetAccelerationStructureHandleNV =                VK_LOAD_FN(device, vkGetAccelerationStructureHandleNV);
    vkBindAccelerationStructureMemoryNV =               VK_LOAD_FN(device, vkBindAccelerationStructureMemoryNV);
    vkGetRayTracingShaderGroupHandlesNV =               VK_LOAD_FN(device, vkGetRayTracingShaderGroupHandlesNV);
    vkGetAccelerationStructureMemoryRequirementsNV =    VK_LOAD_FN(device, vkGetAccelerationStructureMemoryRequirementsNV);
}

} // scatter