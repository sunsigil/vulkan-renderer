#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include "context.h"
#include "device.h"

struct TOS_swapchain
{
	VkSwapchainKHR handle;

	std::vector<VkImage> images;
	VkFormat format;
	VkExtent2D extent;
	std::vector<VkImageView> image_views;
	VkRenderPass render_pass;
	std::vector<VkFramebuffer> framebuffers;

	VkImage depth_image;
	VkDeviceMemory depth_memory;
	VkImageView depth_image_view;
};

void TOS_create_swapchain(TOS_context* context, TOS_device* device, TOS_swapchain* swapchain);
void TOS_destroy_swapchain(TOS_device* device, TOS_swapchain* swapchain);
void TOS_rebuild_swapchain(TOS_context* context, TOS_device* device, TOS_swapchain* swapchain);