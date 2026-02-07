#include "memory.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <iostream>

void* TOS_map_file(const char* path, int mode, size_t* size)
{
	int fd = open(path, O_RDWR, S_IRUSR | S_IWUSR);
	if(fd == -1)
	{
		std::cerr << "TOS_map_file: failed to open " << path << std::endl;
		perror("TOS_map_file: open");
		*size = 0;
		return nullptr;
	}
	size_t file_size = lseek(fd, 0, SEEK_END);
	void* file_data = mmap
	(
		NULL, file_size,
		PROT_READ | PROT_WRITE,
		mode == TOS_FILE_MAP_PUBLIC ? MAP_SHARED : MAP_PRIVATE,
		fd, 0
	);

	close(fd);
	*size = file_size;
	return file_data;
}

void TOS_unmap_file(void* data, size_t size)
{
	munmap(data, size);
}

uint32_t find_memory_type(TOS_device* device, uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(device->physical, &mem_properties);
	for(uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
	{
		if
		(
			(type_filter & (1 << i)) &&
			(mem_properties.memoryTypes[i].propertyFlags & properties) == properties
		)
		{
			return i;
		}
	}
	throw std::runtime_error("[ERROR] failed to find suitable memory type");
}

void TOS_create_buffer
(
	TOS_device* device, VkDeviceSize size,
	VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
	VkBuffer& buffer, VkDeviceMemory& memory
)
{
	VkBufferCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.size = size;
	create_info.usage = usage;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	VkResult result = vkCreateBuffer(device->logical, &create_info, nullptr, &buffer);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_buffer: failed to create buffer");
	
	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(device->logical, buffer, &mem_requirements);
	
	VkMemoryAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = find_memory_type
	(
		device,
		mem_requirements.memoryTypeBits,
		properties
	);
	
	result = vkAllocateMemory(device->logical, &alloc_info, nullptr, &memory);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_buffer: failed to allocate buffer memory");
	
	vkBindBufferMemory(device->logical, buffer, memory, 0);
}

void TOS_copy_buffer
(
	TOS_device* device,
	VkBuffer src, VkBuffer dst,
	VkDeviceSize size
)
{
	VkCommandBuffer command_buffer = TOS_create_command_buffer(device, device->command_pools.transfer);
	TOS_begin_command_buffer(device, command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	
	VkBufferCopy copy_region {};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = size;
	vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);

	TOS_end_command_buffer(device, device->queues.transfer, command_buffer);
	TOS_destroy_command_buffer(device, device->command_pools.transfer, command_buffer);
}

void TOS_create_image
(
	TOS_device* device,
	uint32_t width, uint32_t height, VkFormat format,
	uint32_t mip_levels,
	VkImageTiling tiling, VkImageUsageFlags usage,
	VkMemoryPropertyFlags memory_properties,
	VkImage& image, VkDeviceMemory& memory
)
{
	VkImageCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_info.imageType = VK_IMAGE_TYPE_2D;
	create_info.extent =
	{
		.width = width,
		.height = height,
		.depth = 1
	};
	create_info.mipLevels = mip_levels;
	create_info.arrayLayers = 1;
	create_info.format = format;
	create_info.tiling = tiling;
	create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	create_info.usage = usage;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	create_info.flags = 0;
	
	VkResult result = vkCreateImage(device->logical, &create_info, nullptr, &image);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_image: failed to create image");
	
	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements(device->logical, image, &mem_requirements);
	
	VkMemoryAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = find_memory_type(device, mem_requirements.memoryTypeBits, memory_properties);
	
	result = vkAllocateMemory(device->logical, &alloc_info, nullptr, &memory);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_image: failed to allocate image memory");
	
	vkBindImageMemory(device->logical, image, memory, 0);
}

VkImageView TOS_create_image_view
(
	TOS_device* device, VkImage& image,
	VkFormat format, uint32_t mip_levels,
	VkImageAspectFlags aspects
)
{
	VkImageViewCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.image = image;
	create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	create_info.format = format;
	
	create_info.components =
	{
		.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.a = VK_COMPONENT_SWIZZLE_IDENTITY
	};
	
	create_info.subresourceRange =
	{
		.aspectMask = aspects,
		.baseMipLevel = 0,
		.levelCount = mip_levels,
		.baseArrayLayer = 0,
		.layerCount = 1
	};
	
	VkImageView view;
	VkResult result = vkCreateImageView(device->logical, &create_info, nullptr, &view);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_image_view: failed to create image view");
	
	return view;
}

void TOS_transition_image_layout
(
	TOS_device* device,
	VkImage image, VkFormat format, uint32_t mip_levels,
	VkImageLayout old_layout, VkImageLayout new_layout
)
{
	VkCommandBuffer command_buffer = TOS_create_command_buffer(device, device->command_pools.transfer);
	TOS_begin_command_buffer(device, command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	
	VkImageMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange =
	{
		.baseMipLevel = 0,
		.levelCount = mip_levels,
		.baseArrayLayer = 0,
		.layerCount = 1
	};
	
	if(new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if
		(
			format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
			format == VK_FORMAT_D24_UNORM_S8_UINT
		)
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	
	VkPipelineStageFlags src_stage;
	VkPipelineStageFlags dst_stage;
	if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;	
		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if(old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;	
		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if(old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;	
		src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		
		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		throw std::runtime_error("TOS_transition_image_layout: specified image layout transition not supported");
	}
	
	vkCmdPipelineBarrier
	(
		command_buffer,
		src_stage, dst_stage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
	
	TOS_end_command_buffer(device, device->queues.transfer, command_buffer);
	TOS_destroy_command_buffer(device, device->command_pools.transfer, command_buffer);
}