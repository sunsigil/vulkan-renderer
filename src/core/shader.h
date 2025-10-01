#pragma once

#include <GLFW/glfw3.h>
#include "device.h"

VkShaderModule TOS_load_shader(TOS_device* device, const char* path);