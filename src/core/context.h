#pragma once

#include <GLFW/glfw3.h>
#include <stdint.h>

enum TOS_window_flag
{
	TOS_WINDOW_FLAG_NONE = 0,
	TOS_WINDOW_FLAG_RESIZE = (1 << 0)
};

struct TOS_context
{
	uint32_t window_width;
	uint32_t window_height;
	const char* window_title;
	GLFWwindow* window_handle;
	int window_flags;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkSurfaceKHR surface;
};

void TOS_create_context(TOS_context* context, int width, int height, const char* title);
void TOS_destroy_context(TOS_context* context);