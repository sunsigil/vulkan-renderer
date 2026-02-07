#include "swapchain.h"

#include "device.h"
#include <set>
#include "memory.h"

VkSurfaceFormatKHR select_surface_format(const std::vector<VkSurfaceFormatKHR>& surface_formats)
{
	for(const VkSurfaceFormatKHR& surface_format : surface_formats)
	{
		if(surface_format.format == VK_FORMAT_B8G8R8A8_SRGB && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return surface_format;
	}
	return surface_formats[0];
}

VkPresentModeKHR select_present_mode(const std::vector<VkPresentModeKHR>& present_modes)
{
	for(const VkPresentModeKHR present_mode : present_modes)
	{
		if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return present_mode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D select_surface_extent(TOS_context* context, const VkSurfaceCapabilitiesKHR& surface_capabilities)
{
	VkExtent2D current = surface_capabilities.currentExtent;
	VkExtent2D min = surface_capabilities.minImageExtent;
	VkExtent2D max = surface_capabilities.maxImageExtent;
	
	if(current.width != std::numeric_limits<uint32_t>::max())
		return current;
	
	int width, height;
	glfwGetFramebufferSize(context->window_handle, &width, &height);
	
	VkExtent2D actual = { (uint32_t) width, (uint32_t) height};
	actual.width = std::clamp(actual.width, min.width, max.width);
	actual.height = std::clamp(actual.height, min.height, max.height);
	
	return actual;
}

void create_image_views(TOS_device* device, TOS_swapchain* swapchain)
{
	swapchain->image_views.resize(swapchain->images.size());
	for(int i = 0; i < swapchain->images.size(); i++)
	{
		swapchain->image_views[i] = TOS_create_image_view(device, swapchain->images[i], swapchain->format, 1, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

VkFormat find_supported_depth_format(TOS_device* device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for(VkFormat format : candidates)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(device->physical, format, &properties);
		switch(tiling)
		{
			case VK_IMAGE_TILING_LINEAR:
				if((properties.linearTilingFeatures & features) == features)
					return format;
				break;
			case VK_IMAGE_TILING_OPTIMAL:
				if((properties.optimalTilingFeatures & features) == features)
					return format;
				break;
			default:
				throw std::runtime_error("Desired image tiling mode not suitable for selecting depth image format!");
				break;
		}
	}
	
	throw std::runtime_error("Failed to find supported depth image format!");
}

VkFormat get_depth_format(TOS_device* device)
{
	return find_supported_depth_format
	(
		device,
		{
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkResult create_render_pass(TOS_device* device, TOS_swapchain* swapchain)
{
	VkAttachmentDescription colour_attachment {};
	colour_attachment.format = swapchain->format;
	colour_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colour_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colour_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colour_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colour_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
	VkAttachmentReference colour_attachment_ref {};
	colour_attachment_ref.attachment = 0;
	colour_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkAttachmentDescription depth_attachment {};
	depth_attachment.format = get_depth_format(device);
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
	VkAttachmentReference depth_attachment_ref {};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
	VkSubpassDescription subpass {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colour_attachment_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;
	
	VkAttachmentDescription attachments[] =
	{
		colour_attachment,
		depth_attachment
	};
	
	VkSubpassDependency dependency {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	
	VkRenderPassCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create_info.attachmentCount = 2;
	create_info.pAttachments = attachments;
	create_info.subpassCount = 1;
	create_info.pSubpasses = &subpass;
	create_info.dependencyCount = 1;
	create_info.pDependencies = &dependency;
	
	return vkCreateRenderPass(device->logical, &create_info, nullptr, &swapchain->render_pass);
}

void create_framebuffers(TOS_device* device, TOS_swapchain* swapchain)
{
	swapchain->framebuffers.resize(swapchain->image_views.size());
	for(int i = 0; i < swapchain->framebuffers.size(); i++)
	{
		VkImageView attachments[] = {swapchain->image_views[i], swapchain->depth_image_view};
		
		VkFramebufferCreateInfo create_info {};
		create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		create_info.renderPass = swapchain->render_pass;
		create_info.attachmentCount = 2;
		create_info.pAttachments = attachments;
		create_info.width = swapchain->extent.width;
		create_info.height = swapchain->extent.height;
		create_info.layers = 1;
		
		VkResult result = vkCreateFramebuffer(device->logical, &create_info, nullptr, &swapchain->framebuffers[i]);
		if(result != VK_SUCCESS)
			throw std::runtime_error("[ERROR] failed to create framebuffer");
	}
}

void create_depth_image(TOS_device* device, TOS_swapchain* swapchain)
{
	VkFormat format = get_depth_format(device);
	TOS_create_image
	(
		device,
		swapchain->extent.width, swapchain->extent.height, format,
		1,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		swapchain->depth_image, swapchain->depth_memory
	);
	swapchain->depth_image_view = TOS_create_image_view(device, swapchain->depth_image, format, 1, VK_IMAGE_ASPECT_DEPTH_BIT);
	TOS_transition_image_layout(device, swapchain->depth_image, format, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void TOS_create_swapchain(TOS_context* context, TOS_device* device, TOS_swapchain* swapchain)
{
	TOS_swapchain_support_details support = TOS_query_swapchain_support(context, device->physical);
		
	VkSwapchainCreateInfoKHR create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.oldSwapchain = VK_NULL_HANDLE;
	create_info.surface = context->surface;
	
	VkSurfaceFormatKHR surface_format = select_surface_format(support.surface_formats);
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	
	VkPresentModeKHR present_mode = select_present_mode(support.present_modes);
	create_info.presentMode = present_mode;
	
	VkExtent2D surface_extent = select_surface_extent(context, support.surface_capabilities);
	create_info.imageExtent = surface_extent;
	
	uint32_t image_count = support.surface_capabilities.minImageCount + 1;
	if(support.surface_capabilities.maxImageCount > 0 && image_count > support.surface_capabilities.maxImageCount)
		image_count = support.surface_capabilities.maxImageCount;
	create_info.minImageCount = image_count;

	TOS_queue_family_indices indices = TOS_query_queue_families(context, device->physical);
	std::vector<uint32_t> rendering_indices =
	{
		indices.graphics.value(),
		indices.present.value()
	};
	std::set<uint32_t> unique_indices = std::set<uint32_t>(rendering_indices.begin(), rendering_indices.end());
	if(unique_indices.size() == 1)
	{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}
	else
	{
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = (uint32_t) rendering_indices.size();
		create_info.pQueueFamilyIndices = rendering_indices.data();
	}
	
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.clipped = VK_TRUE;
	create_info.preTransform = support.surface_capabilities.currentTransform;

	VkResult result = vkCreateSwapchainKHR(device->logical, &create_info, nullptr, &swapchain->handle);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_swapchain: failed to create swapchain");
	
	uint32_t swapchain_image_count;
	vkGetSwapchainImagesKHR(device->logical, swapchain->handle, &swapchain_image_count, nullptr);
	swapchain->images.resize(swapchain_image_count);
	vkGetSwapchainImagesKHR(device->logical, swapchain->handle, &swapchain_image_count, swapchain->images.data());
	swapchain->format = create_info.imageFormat;
	swapchain->extent = create_info.imageExtent;

	create_image_views(device, swapchain);
	result = create_render_pass(device, swapchain);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_swapchain: failed to create render pass");
	create_depth_image(device, swapchain);
	create_framebuffers(device, swapchain);	
}

void TOS_destroy_swapchain(TOS_device* device, TOS_swapchain* swapchain)
{
	for(VkFramebuffer framebuffer : swapchain->framebuffers)
	{
		vkDestroyFramebuffer(device->logical, framebuffer, nullptr);
	}

	vkDestroyRenderPass(device->logical, swapchain->render_pass, nullptr);

	vkDestroyImageView(device->logical, swapchain->depth_image_view, nullptr);
	vkFreeMemory(device->logical, swapchain->depth_memory, nullptr);
	vkDestroyImage(device->logical, swapchain->depth_image, nullptr);

	for(VkImageView view : swapchain->image_views)
	{
		vkDestroyImageView(device->logical, view, nullptr);
	}
	vkDestroySwapchainKHR(device->logical, swapchain->handle, nullptr);
}

void TOS_rebuild_swapchain(TOS_context* context, TOS_device* device, TOS_swapchain* swapchain)
{
	int width, height;
	glfwGetFramebufferSize(context->window_handle, &width, &height);
	while(width == 0 || height == 0)
	{
		glfwGetFramebufferSize(context->window_handle, &width, &height);
		glfwWaitEvents();
	}
	
	vkDeviceWaitIdle(device->logical);
	
	TOS_destroy_swapchain(device, swapchain);
	TOS_create_swapchain(context, device, swapchain);
}