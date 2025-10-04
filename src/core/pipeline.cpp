#include "pipeline.h"

#include "shader.h"
#include "swapchain.h"
#include "memory.h"
#include <vulkan/vk_enum_string_helper.h>
#include <iostream>

void TOS_create_uniform_buffer(TOS_device* device, TOS_uniform_buffer* buffer)
{
	VkDeviceSize buffer_size = sizeof(TOS_UBO);
	TOS_create_buffer
	(
		device, buffer_size,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		buffer->buffer, buffer->memory
	);
	vkMapMemory(device->logical, buffer->memory, 0, buffer_size, 0, &buffer->pointer);
}

void TOS_destroy_uniform_buffer(TOS_device* device, TOS_uniform_buffer* buffer)
{
	vkFreeMemory(device->logical, buffer->memory, nullptr);
	vkDestroyBuffer(device->logical, buffer->buffer, nullptr);
}

void TOS_create_descriptor_pipeline(TOS_descriptor_pipeline* pipeline, uint32_t concurrency)
{
	pipeline->concurrency = concurrency;
	pipeline->bindings = std::vector<VkDescriptorSetLayoutBinding>();
	pipeline->layout = VK_NULL_HANDLE;
	pipeline->pool = VK_NULL_HANDLE;
	pipeline->sets = std::vector<VkDescriptorSet>();
}

void TOS_register_descriptor_binding(TOS_descriptor_pipeline* pipeline, VkDescriptorType type, VkShaderStageFlagBits stages)
{
	VkDescriptorSetLayoutBinding binding {};
	binding.binding = pipeline->bindings.size();
	binding.descriptorCount = type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ? MAX_TEXTURE_COUNT : 1;
	binding.descriptorType = type;
	binding.stageFlags = stages;
	binding.pImmutableSamplers = nullptr;

	pipeline->bindings.push_back(binding);
	pipeline->sets.resize(pipeline->bindings.size());
}

void TOS_create_descriptor_layout(TOS_device* device, TOS_descriptor_pipeline* pipeline)
{
	VkDescriptorSetLayoutCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.bindingCount = pipeline->bindings.size();
	create_info.pBindings = pipeline->bindings.data();
	
	VkResult result = vkCreateDescriptorSetLayout(device->logical, &create_info, nullptr, &pipeline->layout);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_descriptor_layout: failed to create descriptor set layout");
}

void TOS_create_descriptor_pool(TOS_device* device, TOS_descriptor_pipeline* pipeline)
{
	std::vector<VkDescriptorPoolSize> pool_sizes = std::vector<VkDescriptorPoolSize>();
	pool_sizes.resize(pipeline->bindings.size());
	for(int i = 0; i < pipeline->bindings.size(); i++)
	{
		pool_sizes[i].type = pipeline->bindings[i].descriptorType;
		pool_sizes[i].descriptorCount = pipeline->concurrency * MAX_TEXTURE_COUNT;
	}
	
	VkDescriptorPoolCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	create_info.poolSizeCount = pool_sizes.size();
	create_info.pPoolSizes = pool_sizes.data();
	create_info.maxSets = pipeline->concurrency;
	
	VkResult result = vkCreateDescriptorPool(device->logical, &create_info, nullptr, &pipeline->pool);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_descriptor_pool: failed to create descriptor pool");
}

void TOS_destroy_descriptor_pipeline(TOS_device* device, TOS_descriptor_pipeline* pipeline)
{
	vkDestroyDescriptorSetLayout(device->logical, pipeline->layout, nullptr);
	vkDestroyDescriptorPool(device->logical,  pipeline->pool, nullptr);
}

void TOS_allocate_descriptor_sets(TOS_device* device, TOS_descriptor_pipeline* pipeline)
{
	std::vector<VkDescriptorSetLayout> layouts;
	layouts.resize(pipeline->concurrency);
	for(int i = 0; i < pipeline->concurrency; i++)
		layouts[i] = pipeline->layout;
	
	VkDescriptorSetAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = pipeline->pool;
	alloc_info.descriptorSetCount = pipeline->concurrency;
	alloc_info.pSetLayouts = layouts.data();
	
	pipeline->sets.resize(pipeline->concurrency);
	VkResult result = vkAllocateDescriptorSets(device->logical, &alloc_info, pipeline->sets.data());
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_allocate_descriptor_sets: failed to allocate descriptor sets");
}

void TOS_update_uniform_buffer_descriptor(TOS_device* device, TOS_descriptor_pipeline* pipeline, uint32_t binding_idx, uint32_t set_idx, TOS_uniform_buffer* buffer)
{
	VkDescriptorBufferInfo info {};
	info.buffer = buffer->buffer;
	info.offset = 0;
	info.range = sizeof(TOS_UBO);
	
	VkWriteDescriptorSet write {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = pipeline->sets[set_idx];
	write.dstBinding = binding_idx;
	write.dstArrayElement = 0;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo = &info;

	vkUpdateDescriptorSets(device->logical, 1, &write, 0, nullptr);
}

void TOS_update_image_sampler_descriptor(TOS_device* device, TOS_descriptor_pipeline* pipeline, uint32_t binding_idx, uint32_t set_idx, TOS_texture* texture)
{
	VkDescriptorImageInfo info[MAX_TEXTURE_COUNT];
	for(int i = 0; i < MAX_TEXTURE_COUNT; i++)
	{
		info[i] = {};
		info[i].imageView = texture[i].view;
		info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info[i].sampler = texture[i].sampler;
	}
	
	VkWriteDescriptorSet write;
	write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = pipeline->sets[set_idx];
	write.dstBinding = binding_idx;
	write.dstArrayElement = 0;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = MAX_TEXTURE_COUNT;
	write.pImageInfo = info;

	vkUpdateDescriptorSets(device->logical, 1, &write, 0, nullptr);
}

VkResult create_render_command_buffers(TOS_device* device, TOS_graphics_pipeline* pipeline)
{
	VkCommandBufferAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = device->command_pools.render;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = MAX_CONCURRENT_FRAMES;
	
	return vkAllocateCommandBuffers(device->logical, &alloc_info, pipeline->render_command_buffers);
}

VkResult create_sync_primitives(TOS_device* device, TOS_graphics_pipeline* pipeline)
{
	VkSemaphoreCreateInfo semaphore_info {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	VkFenceCreateInfo fence_info {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	
	VkResult result = VK_SUCCESS;
	for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
	{
		result = vkCreateSemaphore(device->logical, &semaphore_info, nullptr, &pipeline->image_semaphores[i]);
		if(result != VK_SUCCESS)
			return result;
		result = vkCreateSemaphore(device->logical, &semaphore_info, nullptr, &pipeline->render_semaphores[i]);
		if(result != VK_SUCCESS)
			return result;
		result = vkCreateFence(device->logical, &fence_info, nullptr, &pipeline->frame_fences[i]);
		if(result != VK_SUCCESS)
			return result;
	}

	return VK_SUCCESS;
}

void TOS_create_pipeline(TOS_device* device, TOS_swapchain* swapchain, TOS_descriptor_pipeline* descriptor_pipeline, TOS_graphics_pipeline* pipeline)
{
	VkShaderModule vert_shader = TOS_load_shader(device, "build/assets/shaders/standard.vert.spv");
	VkShaderModule frag_shader = TOS_load_shader(device, "build/assets/shaders/standard.frag.spv");
	
	VkPipelineShaderStageCreateInfo vert_shader_stage_info {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader;
	vert_shader_stage_info.pName = "main";
	VkPipelineShaderStageCreateInfo frag_shader_stage_info {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader;
	frag_shader_stage_info.pName = "main";
	
	VkPipelineVertexInputStateCreateInfo vertex_input_info {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
	VkVertexInputBindingDescription binding_description = TOS_get_vertex_binding_description();
	std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions = TOS_get_vertex_attribute_descriptions();
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = (uint32_t) attribute_descriptions.size();
	vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();
	
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info {};
	input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_info.primitiveRestartEnable = VK_FALSE;
	
	VkViewport viewport {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) swapchain->extent.width;
	viewport.height = (float) swapchain->extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	
	VkRect2D scissor {};
	scissor.offset = {0, 0};
	scissor.extent = swapchain->extent;
	
	VkPipelineViewportStateCreateInfo viewport_info {};
	viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_info.viewportCount = 1;
	viewport_info.pViewports = &viewport;
	viewport_info.scissorCount = 1;
	viewport_info.pScissors = &scissor;
	
	VkPipelineRasterizationStateCreateInfo rasterization_info {};
	rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	
	rasterization_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization_info.depthClampEnable = VK_FALSE;
	
	rasterization_info.depthBiasEnable = VK_FALSE;
	rasterization_info.depthBiasConstantFactor = 0.0f;
	rasterization_info.depthBiasClamp = 0.0f;
	rasterization_info.depthBiasSlopeFactor = 0.0f;
	
	rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_info.lineWidth = 1.0f;
	
	VkPipelineMultisampleStateCreateInfo multisample_info {};
	multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_info.sampleShadingEnable = VK_FALSE;
	multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample_info.minSampleShading = 1.0f;
	multisample_info.pSampleMask = nullptr;
	multisample_info.alphaToCoverageEnable = VK_FALSE;
	multisample_info.alphaToOneEnable = VK_FALSE;
	
	VkPipelineColorBlendAttachmentState colour_blend_attachment {};
	colour_blend_attachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	colour_blend_attachment.blendEnable = VK_TRUE;
	colour_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colour_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colour_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	colour_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colour_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colour_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	
	VkPipelineColorBlendStateCreateInfo colour_blend_info {};
	colour_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colour_blend_info.logicOpEnable = VK_FALSE;
	colour_blend_info.logicOp = VK_LOGIC_OP_COPY;
	colour_blend_info.attachmentCount = 1;
	colour_blend_info.pAttachments = &colour_blend_attachment;
	colour_blend_info.blendConstants[0] = 0.0f;
	colour_blend_info.blendConstants[1] = 0.0f;
	colour_blend_info.blendConstants[2] = 0.0f;
	colour_blend_info.blendConstants[3] = 0.0f;
	
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info {};
	depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_info.depthTestEnable = VK_TRUE;
	depth_stencil_info.depthWriteEnable = VK_TRUE;
	depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_info.stencilTestEnable = VK_FALSE;

	VkPushConstantRange push_constant_range {};
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(TOS_push_constant);
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	VkPipelineLayoutCreateInfo pipeline_layout_info {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_pipeline->layout;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;
	
	VkResult result = vkCreatePipelineLayout(device->logical, &pipeline_layout_info, nullptr, &pipeline->pipeline_layout);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_pipeline: failed to create pipeline layout");
	
	VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};
	
	std::vector<VkDynamicState> dynamic_states =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamic_state_info {};
	dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_info.dynamicStateCount = (uint32_t) dynamic_states.size();
	dynamic_state_info.pDynamicStates = dynamic_states.data();
	
	VkGraphicsPipelineCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	create_info.stageCount = 2;
	create_info.pStages = shader_stages;
	create_info.pVertexInputState = &vertex_input_info;
	create_info.pInputAssemblyState = &input_assembly_info;
	create_info.pViewportState = &viewport_info;
	create_info.pRasterizationState = &rasterization_info;
	create_info.pMultisampleState = &multisample_info;
	create_info.pDepthStencilState = &depth_stencil_info;
	create_info.pColorBlendState = &colour_blend_info;
	create_info.pDynamicState = &dynamic_state_info;
	create_info.layout = pipeline->pipeline_layout;
	create_info.renderPass = swapchain->render_pass;
	create_info.subpass = 0;
	create_info.basePipelineHandle = VK_NULL_HANDLE;
	create_info.basePipelineIndex = 0;
	
	result = vkCreateGraphicsPipelines(device->logical, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline->pipeline);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_pipeline: failed to create pipeline");
	
	vkDestroyShaderModule(device->logical, vert_shader, nullptr);
	vkDestroyShaderModule(device->logical, frag_shader, nullptr);

	result = create_render_command_buffers(device, pipeline);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_pipeline: failed to create render command buffers");

	result = create_sync_primitives(device, pipeline);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_pipeline: failed to create sync primitives");

	pipeline->frame_idx = 0;
}

void TOS_destroy_pipeline(TOS_device* device, TOS_graphics_pipeline* pipeline)
{
	for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
	{
		vkDestroyFence(device->logical, pipeline->frame_fences[i], nullptr);
		vkDestroySemaphore(device->logical,  pipeline->render_semaphores[i], nullptr);
		vkDestroySemaphore(device->logical,  pipeline->image_semaphores[i], nullptr);
	}
	vkDestroyPipeline(device->logical, pipeline->pipeline, nullptr);
	vkDestroyPipelineLayout(device->logical, pipeline->pipeline_layout, nullptr);
}