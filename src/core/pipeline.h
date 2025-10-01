#pragma once

#include <GLFW/glfw3.h>
#include "vertices.h"
#include "textures.h"
#include "swapchain.h"

#define MAX_CONCURRENT_FRAMES 2

struct TOS_UBO
{
	alignas(16) glm::mat4 M;
	alignas(16) glm::mat4 V;
	alignas(16) glm::mat4 P;
};

struct TOS_pipeline
{
	TOS_mesh* mesh;
	TOS_texture* texture;

	VkBuffer uniform_buffers[MAX_CONCURRENT_FRAMES];
	VkDeviceMemory uniform_memories[MAX_CONCURRENT_FRAMES];
	void* uniform_memories_mapped[MAX_CONCURRENT_FRAMES];

	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorPool descriptor_pool;
	std::vector<VkDescriptorSet> descriptor_sets;

	VkPipelineLayout pipeline_layout;
	VkPipeline handle;

	VkCommandBuffer render_command_buffers[MAX_CONCURRENT_FRAMES];

	VkSemaphore image_semaphores[MAX_CONCURRENT_FRAMES];
	VkSemaphore render_semaphores[MAX_CONCURRENT_FRAMES];
	VkFence frame_fences[MAX_CONCURRENT_FRAMES];

	uint32_t frame_idx;
};

void TOS_create_pipeline
(
	TOS_device* device, TOS_swapchain* swapchain, TOS_pipeline* pipeline,
	TOS_mesh* mesh, TOS_texture* texture
);

void TOS_destroy_pipeline(TOS_device* device, TOS_pipeline* pipeline);

void TOS_update_uniforms(TOS_swapchain* swapchain, TOS_pipeline* pipeline);

void TOS_record_render_commands
(
	TOS_swapchain* swapchain, TOS_pipeline* pipeline,
	uint32_t image_index
);

void TOS_draw_frame
(
	TOS_context* context,
	TOS_device* device,
	TOS_swapchain* swapchain,
	TOS_pipeline* pipeline
);