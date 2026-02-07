#pragma once

#include <GLFW/glfw3.h>
#include "device.h"

#define MAX_TEXTURE_COUNT 16
#define TOS_TEXTURE_CHANNELS 4

struct TOS_image
{
	uint32_t width;
	uint32_t height;
	uint8_t depth;
	size_t size;
	uint8_t* pixels;
};

struct TOS_texture
{
	VkDeviceMemory memory;
	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
	VkImage image;
	VkImageView view;
	VkSampler sampler;
};

void TOS_create_image(TOS_image* image, int width, int height);
void TOS_destroy_image(TOS_image* image);
void TOS_load_image(TOS_image* image, const char* path);
void TOS_write_image(TOS_image* image, const char* path);

void TOS_get_pixel(TOS_image* image, int x, int y, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a);
void TOS_set_pixel(TOS_image* image, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void TOS_create_texture(TOS_device* device, TOS_texture* texture, TOS_image* image);
void TOS_destroy_texture(TOS_device* device, TOS_texture* texture);
void TOS_load_texture(TOS_device* device, TOS_texture* texture, const char* path);
void TOS_update_texture(TOS_device* device, TOS_texture* texture, TOS_image* image);
