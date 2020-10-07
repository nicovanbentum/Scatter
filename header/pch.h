#pragma once

#define _CRT_SECURE_NO_WARNINGS

//std libs
#define NOMINMAX
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
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
