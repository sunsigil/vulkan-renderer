#pragma once

#include <GLFW/glfw3.h>

#include "context.h"
#include "device.h"
#include "swapchain.h"

void TOS_create_gui_context(TOS_context* context, TOS_device* device, TOS_swapchain* swapchain);
void TOS_destroy_gui_context();

void TOS_gui_begin_frame();
void TOS_gui_end_frame();

void TOS_gui_begin_overlay();
void TOS_gui_end_overlay();
