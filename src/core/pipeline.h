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

struct TOS_uniform_buffer
{
	VkBuffer buffer;
	VkDeviceMemory memory;
	void* pointer;
};

void TOS_create_uniform_buffer(TOS_device* device, TOS_uniform_buffer* buffer);
void TOS_destroy_uniform_buffer(TOS_device* device, TOS_uniform_buffer* buffer);

struct TOS_descriptor_pipeline
{
	uint32_t concurrency;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	VkDescriptorSetLayout layout;
	VkDescriptorPool pool;
	std::vector<VkDescriptorSet> sets;
};

void TOS_create_descriptor_pipeline(TOS_descriptor_pipeline* pipeline, uint32_t concurrency);
void TOS_destroy_descriptor_pipeline(TOS_device* device, TOS_descriptor_pipeline* pipeline);
void TOS_register_descriptor_binding(TOS_descriptor_pipeline* pipeline, VkDescriptorType type, VkShaderStageFlagBits stages);
void TOS_create_descriptor_layout(TOS_device* device, TOS_descriptor_pipeline* pipeline);
void TOS_create_descriptor_pool(TOS_device* device, TOS_descriptor_pipeline* pipeline);
void TOS_allocate_descriptor_sets(TOS_device* device, TOS_descriptor_pipeline* pipeline);
void TOS_update_uniform_buffer_descriptor(TOS_device* device, TOS_descriptor_pipeline* pipeline, uint32_t binding_idx, uint32_t set_idx, TOS_uniform_buffer* buffer);
void TOS_update_image_sampler_descriptor(TOS_device* device, TOS_descriptor_pipeline* pipeline, uint32_t binding_idx, uint32_t set_idx, TOS_texture* texture);

struct TOS_pipeline
{
	VkPipelineLayout pipeline_layout;
	VkPipeline handle;

	VkCommandBuffer render_command_buffers[MAX_CONCURRENT_FRAMES];

	VkSemaphore image_semaphores[MAX_CONCURRENT_FRAMES];
	VkSemaphore render_semaphores[MAX_CONCURRENT_FRAMES];
	VkFence frame_fences[MAX_CONCURRENT_FRAMES];

	uint32_t frame_idx;
};

void TOS_create_pipeline(TOS_device* device, TOS_swapchain* swapchain, TOS_descriptor_pipeline* descriptor_pipeline, TOS_pipeline* pipeline);
void TOS_destroy_pipeline(TOS_device* device, TOS_pipeline* pipeline);