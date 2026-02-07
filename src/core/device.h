#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include "context.h"

std::vector<VkExtensionProperties> TOS_query_device_extensions(VkPhysicalDevice device);

struct TOS_queue_family_indices
{
	std::optional<uint32_t> transfer;
	std::optional<uint32_t> graphics;
	std::optional<uint32_t> present;
};

TOS_queue_family_indices TOS_query_queue_families(TOS_context* context, VkPhysicalDevice device);

struct TOS_swapchain_support_details
{
	VkSurfaceCapabilitiesKHR surface_capabilities;
	std::vector<VkSurfaceFormatKHR> surface_formats;
	std::vector<VkPresentModeKHR> present_modes;
};

TOS_swapchain_support_details TOS_query_swapchain_support(TOS_context* context, VkPhysicalDevice device);

struct TOS_queues
{
	VkQueue transfer;
	VkQueue graphics;
	VkQueue present;
};

struct TOS_command_pools
{
	VkCommandPool transfer;
	VkCommandPool render;
};

struct TOS_device
{
	VkPhysicalDevice physical;
	VkDevice logical;

	TOS_queues queues;
	TOS_command_pools command_pools;
};

void TOS_create_device(TOS_context* context, TOS_device* device);
void TOS_destroy_device(TOS_context* context, TOS_device* device);

VkCommandBuffer TOS_create_command_buffer(TOS_device* device, VkCommandPool pool);
void TOS_destroy_command_buffer(TOS_device* device, VkCommandPool pool, VkCommandBuffer buffer);
void TOS_begin_command_buffer(TOS_device* device, VkCommandBuffer buffer, VkCommandBufferUsageFlags flags=0);
void TOS_end_command_buffer(TOS_device* device, VkQueue queue, VkCommandBuffer buffer);