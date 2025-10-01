#include "pipeline.h"

#include "shader.h"
#include "swapchain.h"
#include "memory.h"
#include <vulkan/vk_enum_string_helper.h>
#include <iostream>

void create_uniform_buffers(TOS_device* device, TOS_pipeline* pipeline)
{
	VkDeviceSize buffer_size = sizeof(TOS_UBO);
	
	for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
	{
		TOS_create_buffer
		(
			device, buffer_size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			pipeline->uniform_buffers[i], pipeline->uniform_memories[i]
		);
		vkMapMemory(device->logical, pipeline->uniform_memories[i], 0, buffer_size, 0, &pipeline->uniform_memories_mapped[i]);
	}
}

VkResult create_descriptor_set_layout(TOS_device* device, TOS_pipeline* pipeline)
{
	VkDescriptorSetLayoutBinding ubo_binding {};
	ubo_binding.binding = 0;
	ubo_binding.descriptorCount = 1;
	ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo_binding.pImmutableSamplers = nullptr;
	
	VkDescriptorSetLayoutBinding sampler_binding {};
	sampler_binding.binding = 1;
	sampler_binding.descriptorCount = 1;
	sampler_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	sampler_binding.pImmutableSamplers = nullptr;
	
	VkDescriptorSetLayoutBinding bindings[] =
	{
		ubo_binding,
		sampler_binding
	};
	
	VkDescriptorSetLayoutCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.bindingCount = 2;
	create_info.pBindings = bindings;
	
	return vkCreateDescriptorSetLayout(device->logical, &create_info, nullptr, &pipeline->descriptor_set_layout);
}

VkResult create_descriptor_pool(TOS_device* device, TOS_pipeline* pipeline)
{
	VkDescriptorPoolSize ubo_pool_size {};
	ubo_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_pool_size.descriptorCount = MAX_CONCURRENT_FRAMES;
	
	VkDescriptorPoolSize sampler_pool_size {};
	sampler_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_pool_size.descriptorCount = MAX_CONCURRENT_FRAMES;
	
	VkDescriptorPoolSize pool_sizes[] =
	{
		ubo_pool_size,
		sampler_pool_size
	};
	
	VkDescriptorPoolCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	create_info.poolSizeCount = 2;
	create_info.pPoolSizes = pool_sizes;
	create_info.maxSets = MAX_CONCURRENT_FRAMES;
	
	return vkCreateDescriptorPool(device->logical, &create_info, nullptr, &pipeline->descriptor_pool);
}
	
VkResult create_descriptor_sets(TOS_device* device, TOS_pipeline* pipeline)
{
	VkDescriptorSetLayout layouts[MAX_CONCURRENT_FRAMES];
	for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
	{
		layouts[i] = pipeline->descriptor_set_layout;
	}
	
	VkDescriptorSetAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = pipeline->descriptor_pool;
	alloc_info.descriptorSetCount = MAX_CONCURRENT_FRAMES;
	alloc_info.pSetLayouts = layouts;
	
	pipeline->descriptor_sets.resize(MAX_CONCURRENT_FRAMES);
	VkResult result = vkAllocateDescriptorSets(device->logical, &alloc_info, pipeline->descriptor_sets.data());
	if(result != VK_SUCCESS)
		return result;
	
	for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
	{
		VkDescriptorBufferInfo buffer_info {};
		buffer_info.buffer = pipeline->uniform_buffers[i];
		buffer_info.offset = 0;
		buffer_info.range = sizeof(TOS_UBO);
		
		VkWriteDescriptorSet buffer_write {};
		buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		buffer_write.dstSet = pipeline->descriptor_sets[i];
		buffer_write.dstBinding = 0;
		buffer_write.dstArrayElement = 0;
		buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		buffer_write.descriptorCount = 1;
		buffer_write.pBufferInfo = &buffer_info;
		
		VkDescriptorImageInfo image_info {};
		image_info.imageView = pipeline->texture->view;
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.sampler = pipeline->texture->sampler;
		
		VkWriteDescriptorSet image_write {};
		image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		image_write.dstSet = pipeline->descriptor_sets[i];
		image_write.dstBinding = 1;
		image_write.dstArrayElement = 0;
		image_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		image_write.descriptorCount = 1;
		image_write.pImageInfo = &image_info;
		
		VkWriteDescriptorSet writes[] =
		{
			buffer_write,
			image_write
		};
		vkUpdateDescriptorSets(device->logical, 2, writes, 0, nullptr);
	}

	return VK_SUCCESS;
}

VkResult create_render_command_buffers(TOS_device* device, TOS_pipeline* pipeline)
{
	VkCommandBufferAllocateInfo alloc_info {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = device->command_pools.render;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = MAX_CONCURRENT_FRAMES;
	
	return vkAllocateCommandBuffers(device->logical, &alloc_info, pipeline->render_command_buffers);
}

VkResult create_sync_primitives(TOS_device* device, TOS_pipeline* pipeline)
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

void TOS_create_pipeline
(
	TOS_device* device, TOS_swapchain* swapchain, TOS_pipeline* pipeline,
	TOS_mesh* mesh, TOS_texture* texture
)
{
	pipeline->mesh = mesh;
	pipeline->texture = texture;

	create_uniform_buffers(device, pipeline);

	VkResult result = create_descriptor_set_layout(device, pipeline);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_pipeline: failed to create descriptor set layout");
	 result = create_descriptor_pool(device, pipeline);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_pipeline: failed to create descriptor pool");
	result = create_descriptor_sets(device, pipeline);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_create_pipeline: failed to create descriptor sets");

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
	colour_blend_attachment.blendEnable = VK_FALSE;
	colour_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colour_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
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
	
	VkPipelineLayoutCreateInfo pipeline_layout_info {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &pipeline->descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;
	
	result = vkCreatePipelineLayout(device->logical, &pipeline_layout_info, nullptr, &pipeline->pipeline_layout);
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
	
	result = vkCreateGraphicsPipelines(device->logical, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline->handle);
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

void TOS_destroy_pipeline(TOS_device* device, TOS_pipeline* pipeline)
{
	for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
	{
		vkDestroyFence(device->logical, pipeline->frame_fences[i], nullptr);
		vkDestroySemaphore(device->logical,  pipeline->render_semaphores[i], nullptr);
		vkDestroySemaphore(device->logical,  pipeline->image_semaphores[i], nullptr);
	}
	
	for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
	{
		vkFreeMemory(device->logical,  pipeline->uniform_memories[i], nullptr);
		vkDestroyBuffer(device->logical,  pipeline->uniform_buffers[i], nullptr);
	}
	
	vkDestroyPipeline(device->logical, pipeline->handle, nullptr);
	vkDestroyPipelineLayout(device->logical, pipeline->pipeline_layout, nullptr);
	vkDestroyDescriptorSetLayout(device->logical, pipeline->descriptor_set_layout, nullptr);
	vkDestroyDescriptorPool(device->logical,  pipeline->descriptor_pool, nullptr);
}