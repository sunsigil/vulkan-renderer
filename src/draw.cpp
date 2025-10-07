#include "draw.h"

#include "glfw/glfw3.h"
#include "pipeline.h"

static TOS_context* context;
static TOS_device* device;
static TOS_swapchain* swapchain;

TOS_descriptors descriptors;
static TOS_uniform_buffer uniform_buffers[MAX_CONCURRENT_FRAMES];
TOS_texture textures[MAX_TEXTURE_COUNT];
static TOS_work_manager work_manager;

static TOS_pipeline* pipeline;
static uint32_t image_idx;
static VkCommandBuffer command_buffer;

void TOS_create_drawing_context(TOS_context* _context, TOS_device* _device, TOS_swapchain* _swapchain)
{
	context = _context;
	device = _device;
	swapchain = _swapchain;

	for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
		TOS_create_uniform_buffer(device, &uniform_buffers[i]);

	TOS_create_descriptors(&descriptors, MAX_CONCURRENT_FRAMES);
	TOS_register_descriptor_binding(&descriptors, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	TOS_register_descriptor_binding(&descriptors, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	TOS_create_descriptor_layout(device, &descriptors);
	TOS_create_descriptor_pool(device, &descriptors);
	TOS_allocate_descriptor_sets(device, &descriptors);

	for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
	{
		TOS_update_uniform_buffer_descriptor(device, &descriptors, 0, i, &uniform_buffers[i]);
		TOS_update_image_sampler_descriptor(device, &descriptors, 1, i, textures);
	}

	TOS_create_work_manager(device, &work_manager, MAX_CONCURRENT_FRAMES);
}

void TOS_destroy_drawing_context()
{
	TOS_destroy_work_manager(device, &work_manager);
	for(int i = 0; i < MAX_TEXTURE_COUNT; i++)
	{
		if(textures[i].image != VK_NULL_HANDLE)
			TOS_destroy_texture(device, &textures[i]);
	}
	for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
		TOS_destroy_uniform_buffer(device, &uniform_buffers[i]);
	TOS_destroy_descriptors(device, &descriptors);
}

void TOS_begin_frame()
{
	vkWaitForFences(device->logical, 1, &work_manager.frame_fences[work_manager.frame_idx], VK_TRUE, UINT64_MAX);
	VkResult result = vkAcquireNextImageKHR(device->logical, swapchain->handle, UINT64_MAX, work_manager.image_semaphores[work_manager.frame_idx], VK_NULL_HANDLE, &image_idx);
	if(result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		TOS_rebuild_swapchain(context, device, swapchain);
		return;
	}
	else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("TOS_draw_frame: failed to acquire image from swapchain");
	}
	vkResetFences(device->logical, 1, &work_manager.frame_fences[work_manager.frame_idx]);

	command_buffer = work_manager.render_command_buffers[work_manager.frame_idx];
	vkResetCommandBuffer(command_buffer, 0);

	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = nullptr;
	result = vkBeginCommandBuffer(command_buffer, &begin_info);
	if(result != VK_SUCCESS)
		throw std::runtime_error("[ERROR] failed to begin recording render command buffer");
	
	VkRenderPassBeginInfo render_pass_info {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = swapchain->render_pass;
	render_pass_info.framebuffer = swapchain->framebuffers[image_idx];
	render_pass_info.renderArea =
	{
		.offset = {0, 0},
		.extent = swapchain->extent
	};
	render_pass_info.clearValueCount = 2;
	VkClearValue clear_values[] =
	{
		{
			.color = {{0.0f, 0.0f, 0.0f, 1.0f}},
		},
		{
			.depthStencil = {1.0f, 0}
		}
	};
	render_pass_info.pClearValues = clear_values;
	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	
	VkViewport viewport {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) swapchain->extent.width;
	viewport.height = (float) swapchain->extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	
	VkRect2D scissor {};
	scissor.offset = {0, 0};
	scissor.extent = swapchain->extent;
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

VkCommandBuffer TOS_get_command_buffer()
{
	return command_buffer;
}

void TOS_end_frame()
{
	vkCmdEndRenderPass(command_buffer);
	VkResult result = vkEndCommandBuffer(command_buffer);
	if(result != VK_SUCCESS)
		throw std::runtime_error("[ERROR] failed to finish recording render command buffer");
	
	VkSubmitInfo submission {};
	submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submission.commandBufferCount = 1;
	submission.pCommandBuffers = &work_manager.render_command_buffers[work_manager.frame_idx];
	
	submission.waitSemaphoreCount = 1;
	submission.pWaitSemaphores = &work_manager.image_semaphores[work_manager.frame_idx];
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submission.pWaitDstStageMask = wait_stages;
	
	submission.signalSemaphoreCount = 1;
	submission.pSignalSemaphores = &work_manager.render_semaphores[work_manager.frame_idx];
	
	result = vkQueueSubmit(device->queues.graphics, 1, &submission, work_manager.frame_fences[work_manager.frame_idx]);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_draw_frame: failed to submit command buffer");
	
	VkPresentInfoKHR presentation {};
	presentation.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentation.waitSemaphoreCount = 1;
	presentation.pWaitSemaphores = &work_manager.render_semaphores[work_manager.frame_idx];
	
	presentation.swapchainCount = 1;
	presentation.pSwapchains = &swapchain->handle;
	presentation.pImageIndices = &image_idx;
	presentation.pResults = nullptr;
	
	result = vkQueuePresentKHR(device->queues.present, &presentation);
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || (context->window_flags & TOS_WINDOW_FLAG_RESIZE))
	{
		TOS_rebuild_swapchain(context, device, swapchain);
		context->window_flags &= ~TOS_WINDOW_FLAG_RESIZE;
	}
	else if(result != VK_SUCCESS)
	{
		throw std::runtime_error("TOS_draw_frame: failed to present swapchain image");
	}
	
	work_manager.frame_idx = (work_manager.frame_idx + 1) % MAX_CONCURRENT_FRAMES;
}

void TOS_bind_pipeline(TOS_pipeline* _pipeline)
{
	pipeline = _pipeline;
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
}

void TOS_set_UBO(TOS_UBO* ubo)
{
	memcpy(uniform_buffers[work_manager.frame_idx].pointer, ubo, sizeof(TOS_UBO));
}

void TOS_set_push_constants(TOS_push_constants* constants)
{
	vkCmdPushConstants(command_buffer, pipeline->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TOS_push_constants), constants);
}

void TOS_clear_depth_buffer()
{
	VkClearValue clear_value {};
	clear_value.depthStencil = {1.0f, 0};
	VkClearAttachment depth_attachment {};
	depth_attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depth_attachment.clearValue = clear_value;
	VkClearRect clear_rect {};
	clear_rect.baseArrayLayer = 0;
	clear_rect.layerCount = 1;
	clear_rect.rect = VkRect2D {.offset = {0, 0}, .extent = swapchain->extent};
	vkCmdClearAttachments(command_buffer, 1, &depth_attachment, 1, &clear_rect);
}

void TOS_draw_mesh(TOS_mesh* mesh)
{
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &mesh->vertex_buffer, &offset);
	vkCmdBindIndexBuffer(command_buffer, mesh->index_buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline_layout, 0, 1, &descriptors.sets[work_manager.frame_idx], 0, nullptr);
	vkCmdDrawIndexed(command_buffer, (uint32_t) mesh->indices.size(), 1, 0, 0, 0);
}