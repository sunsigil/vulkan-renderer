#pragma once

#include "context.h"
#include "device.h"
#include "swapchain.h"
#include "pipeline.h"

extern TOS_descriptors descriptors;
extern TOS_texture textures[MAX_TEXTURE_COUNT];

void TOS_create_drawing_context(TOS_context* context, TOS_device* device, TOS_swapchain* swapchain);
void TOS_destroy_drawing_context();

void TOS_begin_frame();
VkCommandBuffer TOS_get_command_buffer();
void TOS_end_frame();

void TOS_bind_pipeline(TOS_pipeline* pipeline);
void TOS_set_UBO(TOS_UBO* ubo);
void TOS_set_push_constants(TOS_push_constants* constants);

void TOS_clear_depth_buffer();
void TOS_draw_mesh(TOS_mesh* mesh);