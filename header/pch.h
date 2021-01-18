#pragma once

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef NDEBUG
#define IS_DEBUG 1
#else
#define IS_DEBUG 0
#endif

#define NOMINMAX
#define _USE_MATH_DEFINES
#include <Windows.h>
#include <cmath>

//std libs
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <assert.h>
#include <optional>
#include <set>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <filesystem>

#include "vulkan/vulkan.h"
#include "vulkan/vulkan_win32.h"

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif 

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtx/euler_angles.hpp"

// stupid libraries
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"
