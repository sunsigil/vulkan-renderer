#pragma once

#include <GLFW/glfw3.h>
#include "device.h"
#include <stdint.h>
#include <stddef.h>

enum TOS_file_map_mode
{
	TOS_FILE_MAP_PRIVATE,
	TOS_FILE_MAP_PUBLIC
};

void* TOS_map_file(const char* path, int mode, size_t* size);
void TOS_unmap_file(void* data, size_t size);

void TOS_create_buffer
(
	TOS_device* device, VkDeviceSize size,
	VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
	VkBuffer& buffer, VkDeviceMemory& memory
);

void TOS_copy_buffer(TOS_device* device, VkBuffer src, VkBuffer dst, VkDeviceSize size);

void TOS_create_image
(
	TOS_device* device,
	uint32_t width, uint32_t height, VkFormat format,
	uint32_t mip_levels,
	VkImageTiling tiling, VkImageUsageFlags usage,
	VkMemoryPropertyFlags memory_properties,
	VkImage& image, VkDeviceMemory& memory
);

VkImageView TOS_create_image_view
(
	TOS_device* device, VkImage& image,
	VkFormat format, uint32_t mip_levels,
	VkImageAspectFlags aspects
);

void TOS_transition_image_layout
(
	TOS_device* device,
	VkImage image, VkFormat format, uint32_t mip_levels,
	VkImageLayout old_layout, VkImageLayout new_layout
);