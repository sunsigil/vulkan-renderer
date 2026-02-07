#include "device.h"

#include <set>
#include <iostream>

std::vector<VkExtensionProperties> TOS_query_device_extensions(VkPhysicalDevice device)
{
	uint32_t count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
	std::vector<VkExtensionProperties> extensions(count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
	return extensions;
}

TOS_queue_family_indices TOS_query_queue_families(TOS_context* context, VkPhysicalDevice device)
{
	uint32_t family_count;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);
	std::vector<VkQueueFamilyProperties> family_properties;
	family_properties.resize(family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, family_properties.data());
	
	TOS_queue_family_indices indices;
	for(int i = 0; i < family_count; i++)
	{
		if(family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			indices.transfer = i;
		
		if(family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphics = i;
		
		VkBool32 present_support;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context->surface, &present_support);
		if(present_support)
			indices.present = i;
	}
	
	return indices;
}

TOS_swapchain_support_details TOS_query_swapchain_support(TOS_context* context, VkPhysicalDevice device)
{
	TOS_swapchain_support_details details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context->surface, &details.surface_capabilities);
	
	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, context->surface, &format_count, nullptr);
	if(format_count > 0)
	{
		details.surface_formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, context->surface, &format_count, details.surface_formats.data());
	}
	
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, context->surface, &present_mode_count, nullptr);
	if(present_mode_count > 0)
	{
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, context->surface, &present_mode_count, details.present_modes.data());
	}
	
	return details;
}

bool is_device_suitable(TOS_context* context, VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties properties {};
	vkGetPhysicalDeviceProperties(device, &properties);

	VkPhysicalDeviceFeatures features {};
	vkGetPhysicalDeviceFeatures(device, &features);
	if(!features.samplerAnisotropy)
		return false;

	VkPhysicalDeviceFeatures2 features2 {};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR requested_fragment_shader_barycentric_features {};
	requested_fragment_shader_barycentric_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR;
	requested_fragment_shader_barycentric_features.fragmentShaderBarycentric = VK_TRUE;
	features2.pNext = &requested_fragment_shader_barycentric_features;
	vkGetPhysicalDeviceFeatures2(device, &features2);
	
	TOS_queue_family_indices queue_family_indices = TOS_query_queue_families(context, device);
	if(!queue_family_indices.transfer.has_value())
		return false;
	if(!queue_family_indices.graphics.has_value())
		return false;
	if(!queue_family_indices.present.has_value())
		return false;
	
	std::vector<VkExtensionProperties> supported_extensions = TOS_query_device_extensions(device);
	std::vector<const char*> required_extensions =
	{
		"VK_KHR_portability_subset",
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		"VK_KHR_fragment_shader_barycentric"
	};
	for(int i = 0; i < required_extensions.size(); i++)
	{
		bool required_is_supported = false;
		for(int j = 0; j < supported_extensions.size(); j++)
		{
			if(!strcmp(required_extensions[i], supported_extensions[j].extensionName))
			{
				required_is_supported = true;
				break;
			}
		}
		if(!required_is_supported)
			return false;
	}
	
	TOS_swapchain_support_details swapchain_support = TOS_query_swapchain_support(context, device);
	if(swapchain_support.surface_formats.empty())
		return false;
	if(swapchain_support.present_modes.empty())
		return false;
	
	return true;
}

VkResult select_physical_device(TOS_context* context, VkPhysicalDevice* device)
{
	uint32_t count;
	vkEnumeratePhysicalDevices(context->instance, &count, nullptr);
	if(count <= 0)
		return VK_ERROR_UNKNOWN;

	std::vector<VkPhysicalDevice> devices;
	devices.resize(count);
	vkEnumeratePhysicalDevices(context->instance, &count, devices.data());
	for(int i = 0; i < count; i++)
	{
		VkPhysicalDevice candidate = devices[i];
		if(is_device_suitable(context, candidate))
		{
			*device = candidate;
			return VK_SUCCESS;
		}
	}

	return VK_ERROR_UNKNOWN;
}

VkResult create_logical_device(TOS_context* context, VkPhysicalDevice physical, VkDevice* logical)
{
	VkDeviceCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	
	TOS_queue_family_indices queue_family_indices = TOS_query_queue_families(context, physical);
	std::set<uint32_t> unique_queue_families =
	{
		queue_family_indices.transfer.value(),
		queue_family_indices.graphics.value(),
		queue_family_indices.present.value()
	};
	
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families)
	{
		queue_create_infos.push_back
		({
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queue_family,
			.queueCount = 1,
			.pQueuePriorities = &queue_priority
		});
	}
	
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.queueCreateInfoCount = (uint32_t) queue_create_infos.size();
	
	VkPhysicalDeviceFeatures2 features2 {};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.features.samplerAnisotropy = VK_TRUE;
	VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR requested_fragment_shader_barycentric_features {};
	requested_fragment_shader_barycentric_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR;
	requested_fragment_shader_barycentric_features.fragmentShaderBarycentric = VK_TRUE;
	features2.pNext = &requested_fragment_shader_barycentric_features;
	create_info.pNext = &features2;
	
	std::vector<const char*> required_extensions =
	{
		"VK_KHR_portability_subset",
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		"VK_KHR_fragment_shader_barycentric"
	};
	create_info.ppEnabledExtensionNames = required_extensions.data();
	create_info.enabledExtensionCount = (uint32_t) required_extensions.size();
	
	return vkCreateDevice(physical, &create_info, nullptr, logical);
}

void create_queues(TOS_context* context, TOS_device* device)
{
	TOS_queue_family_indices indices = TOS_query_queue_families(context, device->physical);
	vkGetDeviceQueue(device->logical, indices.transfer.value(), 0, &device->queues.transfer);
	vkGetDeviceQueue(device->logical, indices.graphics.value(), 0, &device->queues.graphics);
	vkGetDeviceQueue(device->logical, indices.present.value(), 0, &device->queues.present);
}

void create_command_pools(TOS_context* context, TOS_device* device)
{
	TOS_queue_family_indices indices = TOS_query_queue_families(context, device->physical);
		
	VkCommandPoolCreateInfo transfer_pool_info {};
	transfer_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transfer_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	transfer_pool_info.queueFamilyIndex = indices.transfer.value();
	VkResult result = vkCreateCommandPool(device->logical, &transfer_pool_info, nullptr, &device->command_pools.transfer);
	if(result != VK_SUCCESS)
		throw std::runtime_error("[ERROR] failed to create transfer command pool");
	
	VkCommandPoolCreateInfo render_pool_info {};
	render_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	render_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	render_pool_info.queueFamilyIndex = indices.graphics.value();
	result = vkCreateCommandPool(device->logical, &render_pool_info, nullptr, &device->command_pools.render);
	if(result != VK_SUCCESS)
		throw std::runtime_error("[ERROR] failed to create render command pool");
}

void TOS_create_device(TOS_context* context, TOS_device* device)
{
	VkResult result = select_physical_device(context, &device->physical);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_device: failed to select physical device");

	result = create_logical_device(context, device->physical, &device->logical);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_device: failed to create logical device");

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device->physical, &properties);
	std::cout << "TOS_create_device: selected:\n\t" << properties.deviceName << std::endl;

	create_queues(context, device);
	create_command_pools(context, device);
}

void TOS_destroy_device(TOS_context* context, TOS_device* device)
{
	vkDestroyCommandPool(device->logical, device->command_pools.render, nullptr);
	vkDestroyCommandPool(device->logical, device->command_pools.transfer, nullptr);
	vkDestroyDevice(device->logical, nullptr);
}

VkCommandBuffer TOS_create_command_buffer(TOS_device* device, VkCommandPool pool)
{
	VkCommandBufferAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = pool;
	alloc_info.commandBufferCount = 1;
	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device->logical, &alloc_info, &command_buffer);
	return command_buffer;
}

void TOS_destroy_command_buffer(TOS_device* device, VkCommandPool pool, VkCommandBuffer buffer)
{
	vkFreeCommandBuffers(device->logical, pool, 1, &buffer);
}

void TOS_begin_command_buffer(TOS_device* device, VkCommandBuffer buffer, VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = flags;
	begin_info.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(buffer, &begin_info);
}

void TOS_end_command_buffer(TOS_device* device, VkQueue queue, VkCommandBuffer buffer)
{
	vkEndCommandBuffer(buffer);
	VkSubmitInfo submission {};
	submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submission.commandBufferCount = 1;
	submission.pCommandBuffers = &buffer;
	vkQueueSubmit(queue, 1, &submission, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);
}