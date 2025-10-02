#include "context.h"

#include <stdexcept>
#include <iostream>

static void resize_callback(GLFWwindow* handle, int width, int height)
{
	TOS_context* context = (TOS_context*) glfwGetWindowUserPointer(handle);
	context->window_flags |= TOS_WINDOW_FLAG_RESIZE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback
(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data
)
{
	if(severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		switch (severity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				std::cerr << "[ERROR] ";
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				std::cerr << "[WARNING] ";
				break;
			default:
				break;
		}
		std::cerr << callback_data->pMessage << std::endl;
	}
	
	return VK_FALSE;
}

VkResult create_vulkan_instance(TOS_context* context)
{
	VkInstanceCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	
	VkApplicationInfo app_info {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = context->window_title;
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;
	create_info.pApplicationInfo = &app_info;
	
	VkDebugUtilsMessengerCreateInfoEXT create_debug_info {};
	create_debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_debug_info.pfnUserCallback = debug_callback;
	create_info.pNext = &create_debug_info;
	
	std::vector<const char*> required_extensions;
	required_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	required_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	uint32_t glfw_required_extension_count;
	const char** glfw_required_extensions;
	glfw_required_extensions = glfwGetRequiredInstanceExtensions(&glfw_required_extension_count);
	for(int i = 0; i < glfw_required_extension_count; i++)
	{
		required_extensions.push_back(glfw_required_extensions[i]);
	}
	
	create_info.enabledExtensionCount = (uint32_t) required_extensions.size();
	create_info.ppEnabledExtensionNames = required_extensions.data();
	// Let's see what extensions we're loading
	std::cout << "Enabled extensions:" << std::endl;
	for(int i = 0; i < create_info.enabledExtensionCount; i++)
	{
		std::cout << "\t" << create_info.ppEnabledExtensionNames[i] << std::endl;
	}
	
	std::vector<const char*> required_layers =
	{
		"VK_LAYER_KHRONOS_validation"
	};
	
	uint32_t available_layer_count;
	vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);
	std::vector<VkLayerProperties> available_layers;
	available_layers.resize(available_layer_count);
	
	for(int i = 0; i < required_layers.size(); i++)
	{
		const char* requested = required_layers[i];
		bool requested_is_available = false;
		for(int j = 0; j < available_layer_count; j++)
		{
			const char* available = available_layers[j].layerName;
			if(strcmp(requested, available))
			{
				requested_is_available = true;
				break;
			}
		}
		if(!requested_is_available)
			throw std::runtime_error("Missing validation layer!");
	}
	
	create_info.enabledLayerCount = (uint32_t) required_layers.size();
	create_info.ppEnabledLayerNames = required_layers.data();
	// Let's see what layers we're loading
	std::cout << "Enabled layers:" << std::endl;
	for(int i = 0; i < create_info.enabledLayerCount; i++)
	{
		std::cout << "\t" << create_info.ppEnabledLayerNames[i] << std::endl;
	}
	
	return vkCreateInstance(&create_info, nullptr, &context->instance);
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT function = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if(function != nullptr)
		return function(instance, pCreateInfo, pAllocator, pDebugMessenger);
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

VkResult create_debug_messenger(TOS_context* context)
{
	VkDebugUtilsMessengerCreateInfoEXT create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debug_callback;
	
	return CreateDebugUtilsMessengerEXT(context->instance, &create_info, nullptr, &context->debug_messenger);
}

void TOS_create_context(TOS_context* context, int width, int height, const char* title)
{
	if(!glfwInit())
		throw std::runtime_error("TOS_create_context: failed to initialize GLFW");
		
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	
	context->window_width = width;
	context->window_height = height;
	context->window_title = title;
	context->window_handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
	glfwSetWindowUserPointer(context->window_handle, context);

	glfwSetFramebufferSizeCallback(context->window_handle, resize_callback);

	VkResult result = create_vulkan_instance(context);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_context: failed to create Vulkan instance");

	result = create_debug_messenger(context);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_context: failed to create debug messenger");

	result = glfwCreateWindowSurface(context->instance, context->window_handle, nullptr, &context->surface);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_context: failed to create surface with GLFW");
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	PFN_vkDestroyDebugUtilsMessengerEXT function = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if(function != nullptr)
		return function(instance, debugMessenger, pAllocator);
}

void TOS_destroy_context(TOS_context* context)
{
	vkDestroySurfaceKHR(context->instance, context->surface, nullptr);

	DestroyDebugUtilsMessengerEXT(context->instance, context->debug_messenger, nullptr);
	vkDestroyInstance(context->instance, nullptr);

	glfwDestroyWindow(context->window_handle);
	glfwTerminate();
}