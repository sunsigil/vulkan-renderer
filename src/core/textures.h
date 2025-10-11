#pragma once

#include <GLFW/glfw3.h>
#include "device.h"

#define MAX_TEXTURE_COUNT 8

struct TOS_image_asset
{
	uint32_t width;
	uint32_t height;
	uint32_t channels;
	uint8_t depth;
	size_t size;
	uint8_t* pixels;
};

struct TOS_texture
{
	VkDeviceMemory memory;
	VkImage image;
	VkImageView view;
	VkSampler sampler;
};

void TOS_load_image_asset(TOS_image_asset* image, const char* path);
void TOS_destroy_image_asset(TOS_image_asset* image);
void TOS_write_image_asset(TOS_image_asset* image, const char* path);

void TOS_load_texture(TOS_device* device, TOS_texture* texture, const char* path);
void TOS_destroy_texture(TOS_device* device, TOS_texture* texture);