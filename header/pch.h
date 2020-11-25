#pragma once

#define _CRT_SECURE_NO_WARNINGS

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
#include <variant>

#include "vulkan/vulkan.h"
#include "vulkan/vulkan_win32.h"

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtx/euler_angles.hpp"

// stupid libraries
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"
