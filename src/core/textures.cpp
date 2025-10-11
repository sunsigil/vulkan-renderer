#include "textures.h"

#include "memory.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void TOS_load_image_asset(TOS_image_asset* image, const char* path)
{
	int width, height, channels;
	stbi_uc* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
	if(pixels == nullptr)
		throw std::runtime_error("TOS_load_image: STBI failed to load image");
	size_t size = width * height * 4;
	*image = 
	{
		.width = (uint32_t) width,
		.height = (uint32_t) height,
		.channels = (uint32_t) channels,
		.size = size,
		.pixels = pixels
	};
}

void TOS_destroy_image_asset(TOS_image_asset* image)
{
	stbi_image_free(image->pixels);
}

void TOS_write_image_asset(TOS_image_asset* image, const char* path)
{
	stbi_write_png
	(
		path,
		image->width, image->height, image->channels,
		image->pixels,
		image->width * image->channels
	);
}

void copy_buffer_to_image
(
	TOS_device* device,
	VkBuffer buffer, VkImage image,
	uint32_t width, uint32_t height
)
{
	VkCommandBuffer command_buffer = TOS_begin_transfer_command_buffer(device);
	
	VkBufferImageCopy copy_region {};
	copy_region.bufferOffset = 0;
	copy_region.bufferRowLength = 0;
	copy_region.bufferImageHeight = 0;
	
	copy_region.imageSubresource =
	{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.mipLevel = 0,
		.baseArrayLayer = 0,
		.layerCount = 1
	};
	
	copy_region.imageOffset = {0, 0, 0};
	copy_region.imageExtent = {width, height, 1};
	
	vkCmdCopyBufferToImage
	(
		command_buffer, buffer, image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &copy_region
	);
	
	TOS_end_transfer_command_buffer(device, &command_buffer);
}
	
void generate_mipmaps
(
	TOS_device* device,
	VkImage image, VkFormat format,
	int32_t width, int32_t height,
	uint32_t mip_levels
)
{
	VkFormatProperties format_properties;
	vkGetPhysicalDeviceFormatProperties(device->physical, format, &format_properties);
	if(!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		throw std::runtime_error("Requested mipmap format does not support linear filtering!");
	
	VkCommandBuffer command_buffer = TOS_begin_transfer_command_buffer(device);
	
	VkImageMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange =
	{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseArrayLayer = 0,
		.layerCount = 1,
		.levelCount = 1
	};
	
	int32_t mip_width = width;
	int32_t mip_height = height;
	for(uint32_t level = 1; level < mip_levels; level++)
	{
		barrier.subresourceRange.baseMipLevel = level - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		
		vkCmdPipelineBarrier
		(
			command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
		
		VkImageBlit blit {};
		blit.srcOffsets[0] = {0, 0, 0};
		blit.srcOffsets[1] = {mip_width, mip_height, 1};
		blit.srcSubresource =
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = level - 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};
		blit.dstOffsets[0] = {0, 0, 0};
		blit.dstOffsets[1] =
		{
			mip_width > 1 ? mip_width / 2 : 1,
			mip_height > 1 ? mip_height / 2 : 1,
			1
		};
		blit.dstSubresource =
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = level,
			.baseArrayLayer = 0,
			.layerCount = 1
		};
		
		vkCmdBlitImage
		(
			command_buffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit, VK_FILTER_LINEAR
		);
		
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		vkCmdPipelineBarrier
		(
			command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
		
		if(mip_width > 1)
			mip_width /= 2;
		if(mip_height > 1)
			mip_height /= 2;
	}
	
	barrier.subresourceRange.baseMipLevel = mip_levels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	
	vkCmdPipelineBarrier
	(
		command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
	
	TOS_end_transfer_command_buffer(device, &command_buffer);
}
	
VkResult create_sampler(TOS_device* device, TOS_texture* texture, int mip_levels)
{
	VkSamplerCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.magFilter = VK_FILTER_LINEAR;
	create_info.minFilter = VK_FILTER_LINEAR;
	create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	
	VkPhysicalDeviceProperties hardware;
	vkGetPhysicalDeviceProperties(device->physical, &hardware);
	create_info.anisotropyEnable = VK_TRUE;
	create_info.maxAnisotropy = hardware.limits.maxSamplerAnisotropy;
	
	create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	create_info.unnormalizedCoordinates = VK_FALSE;
	create_info.compareEnable = VK_FALSE;
	create_info.compareOp = VK_COMPARE_OP_ALWAYS;
	create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	create_info.mipLodBias = 0.0f;
	create_info.minLod = 0.0f;
	create_info.maxLod = (float) mip_levels;
	
	return vkCreateSampler(device->logical, &create_info, nullptr, &texture->sampler);
}

void TOS_load_texture(TOS_device* device, TOS_texture* texture, const char* path)
{
	TOS_image_asset image;
	TOS_load_image_asset(&image, path);
	
	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
	TOS_create_buffer
	(
		device, image.size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer, staging_memory
	);
	void* data;
	vkMapMemory(device->logical, staging_memory, 0, image.size, 0, &data);
	memcpy(data, image.pixels, image.size);
	vkUnmapMemory(device->logical, staging_memory);
	
	int mip_levels = std::floor(std::log2(std::max(image.width, image.height))) + 1;
	TOS_create_image
	(
		device,
		image.width, image.height, VK_FORMAT_R8G8B8A8_SRGB,
		mip_levels, VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		texture->image, texture->memory
	);
	TOS_transition_image_layout(device, texture->image, VK_FORMAT_R8G8B8A8_SRGB, mip_levels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copy_buffer_to_image(device, staging_buffer, texture->image, image.width, image.height);
	generate_mipmaps(device, texture->image, VK_FORMAT_R8G8B8A8_SRGB, image.width, image.height, mip_levels);

	texture->view = TOS_create_image_view(device, texture->image, VK_FORMAT_R8G8B8A8_SRGB, mip_levels, VK_IMAGE_ASPECT_COLOR_BIT);
	create_sampler(device, texture, mip_levels);
	
	vkFreeMemory(device->logical, staging_memory, nullptr);
	vkDestroyBuffer(device->logical, staging_buffer, nullptr);

	TOS_destroy_image_asset(&image);
}

void TOS_destroy_texture(TOS_device* device, TOS_texture* texture)
{
	vkDestroySampler(device->logical, texture->sampler, nullptr);
	vkDestroyImageView(device->logical, texture->view, nullptr);
	vkFreeMemory(device->logical, texture->memory, nullptr);
	vkDestroyImage(device->logical, texture->image, nullptr);
}