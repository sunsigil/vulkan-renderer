#pragma once

#include <GLFW/glfw3.h>
#include "device.h"

struct TOS_texture
{
	uint32_t mip_levels;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkSampler sampler;
};

void TOS_load_texture(TOS_device* device, TOS_texture* texture, const char* path);
void TOS_destroy_texture(TOS_device* device, TOS_texture* texture);