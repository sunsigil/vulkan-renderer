#pragma once

#include <GLFW/glfw3.h>
#include "vertices.h"
#include "textures.h"
#include "swapchain.h"

#define MAX_CONCURRENT_FRAMES 2
#define MAX_TEXTURE_COUNT 8

struct TOS_UBO
{
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

struct TOS_push_constant
{
	glm::mat4 M;
	int texture_idx;
	float wireframe;
};

struct TOS_descriptors
{
	uint32_t concurrency;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	VkDescriptorSetLayout layout;
	VkDescriptorPool pool;
	std::vector<VkDescriptorSet> sets;
};

void TOS_create_descriptors(TOS_descriptors* pipeline, uint32_t concurrency);
void TOS_destroy_descriptors(TOS_device* device, TOS_descriptors* pipeline);
void TOS_register_descriptor_binding(TOS_descriptors* pipeline, VkDescriptorType type, VkShaderStageFlagBits stages);
void TOS_create_descriptor_layout(TOS_device* device, TOS_descriptors* pipeline);
void TOS_create_descriptor_pool(TOS_device* device, TOS_descriptors* pipeline);
void TOS_allocate_descriptor_sets(TOS_device* device, TOS_descriptors* pipeline);
void TOS_update_uniform_buffer_descriptor(TOS_device* device, TOS_descriptors* pipeline, uint32_t binding_idx, uint32_t set_idx, TOS_uniform_buffer* buffer);
void TOS_update_image_sampler_descriptor(TOS_device* device, TOS_descriptors* pipeline, uint32_t binding_idx, uint32_t set_idx, TOS_texture* texture);

struct TOS_pipeline_specification
{
	const char* vert_path;
	const char* frag_path;
	VkPrimitiveTopology topology;
	VkPolygonMode polygon_mode;
	VkCompareOp depth_compare_op;
};

struct TOS_pipeline
{
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
};

void TOS_create_pipeline
(
	TOS_device* device, TOS_swapchain* swapchain,
	TOS_descriptors* descriptors, TOS_pipeline_specification specification,
	TOS_pipeline* pipeline
);
void TOS_destroy_pipeline(TOS_device* device, TOS_pipeline* pipeline);

struct TOS_work_manager
{
	uint32_t concurrency;
	VkCommandBuffer* render_command_buffers;
	VkSemaphore* image_semaphores;
	VkSemaphore* render_semaphores;
	VkFence* frame_fences;
	uint32_t frame_idx;
};

void TOS_create_work_manager(TOS_device* device, TOS_work_manager* manager, uint32_t concurrency);
void TOS_destroy_work_manager(TOS_device* device, TOS_work_manager* manager);