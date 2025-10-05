#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include "context.h"
#include "device.h"
#include "swapchain.h"
#include "pipeline.h"
#include "input.h"
#include "gui.h"
#include "timing.h"
#include "machines.h"
#include "camera.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include "glm/gtx/string_cast.hpp"

static TOS_context context;
static TOS_device device;
static TOS_swapchain swapchain;

static TOS_descriptor_pipeline descriptor_pipeline;
static TOS_uniform_buffer uniform_buffers[MAX_CONCURRENT_FRAMES];
static TOS_mesh mesh;
static TOS_mesh aabb;
static TOS_texture textures[MAX_TEXTURE_COUNT];

static TOS_graphics_pipeline pipeline;

static TOS_camera camera;
static TOS_UBO uniforms;
static TOS_push_constant push_constant;
static bool show_gui;
static TOS_latch wireframe_latch(false);
static TOS_timeline wireframe_timeline(0.5, true);
static bool show_aabb;
static TOS_transform model;

void logic_init()
{
	TOS_create_timing_context();
	TOS_create_input_context(&context);

	camera = TOS_camera
	(
		glm::vec3(0, 0, -3), glm::vec3(0),
		M_PI/4, (float) swapchain.extent.width / (float) swapchain.extent.height, 0.01f, 1000.0f
	);

	TOS_toggle_cursor(false);
}

void logic_tick()
{
	// PRE-TICKS

	TOS_timing_tick();
	TOS_input_tick();

	// INPUT-CONTROLLED

	if(TOS_key_down(GLFW_KEY_LEFT_SHIFT) && TOS_key_pressed(GLFW_KEY_TAB))
	{
		show_gui = !show_gui;
		TOS_toggle_cursor(show_gui);
	}

	if(!show_gui)
	{
		glm::vec3 walk = glm::vec3(0);
		if(TOS_key_down(GLFW_KEY_W))
			walk += camera.transform.forward();
		if(TOS_key_down(GLFW_KEY_S))
			walk -= camera.transform.forward();
		if(TOS_key_down(GLFW_KEY_D))
			walk -= camera.transform.right();
		if(TOS_key_down(GLFW_KEY_A))
			walk += camera.transform.right();
		camera.transform.velocity = 7.0f * walk;

		glm::vec2 dmouse = TOS_mouse_delta();
		dmouse.x /= context.window_width;
		dmouse.y /= context.window_height;
		camera.rotate(dmouse.y, -dmouse.x);
	}

	show_aabb = false;
	if(TOS_mouse_down())
	{
		glm::vec2 mouse = TOS_mouse_position();
		mouse.x /= context.window_width;
		mouse.y /= context.window_height;
		TOS_ray ray = camera.viewport_ray(mouse.x, mouse.y);

		TOS_AABB aabb = TOS_AABB::min_max(mesh.min, mesh.max);

		std::optional<TOS_raycast_hit> hit = TOS_ray_OBB_intersect(ray, aabb, model.M());
		if(hit.has_value())
			show_aabb = true;
	}

	// GUI-CONTROLLED

	if(wireframe_latch.flipped())
	{
		wireframe_timeline.reset();
		wireframe_timeline.resume();
		wireframe_timeline.reverse();
	}

	// EFFECTS
	model.orientation.y += TOS_get_delta_time_s();
	uniforms.V = camera.V();
	uniforms.P = camera.P();
	uniforms.P[1][1] *= -1.0f;
	memcpy(uniform_buffers[pipeline.frame_idx].pointer, &uniforms, sizeof(uniforms));

	// POST-TICKS

	wireframe_timeline.tick();
	wireframe_latch.tick();
	camera.tick();
}

void begin_frame(VkCommandBuffer command_buffer, uint32_t image_index)
{
	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = nullptr;
	VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
	if(result != VK_SUCCESS)
		throw std::runtime_error("[ERROR] failed to begin recording render command buffer");
	
	VkRenderPassBeginInfo render_pass_info {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = swapchain.render_pass;
	render_pass_info.framebuffer = swapchain.framebuffers[image_index];
	render_pass_info.renderArea =
	{
		.offset = {0, 0},
		.extent = swapchain.extent
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
	
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
	
	VkViewport viewport {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) swapchain.extent.width;
	viewport.height = (float) swapchain.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	
	VkRect2D scissor {};
	scissor.offset = {0, 0};
	scissor.extent = swapchain.extent;
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void end_frame(VkCommandBuffer command_buffer)
{
	vkCmdEndRenderPass(command_buffer);
	VkResult result = vkEndCommandBuffer(command_buffer);
	if(result != VK_SUCCESS)
		throw std::runtime_error("[ERROR] failed to finish recording render command buffer");
}

void draw_mesh(VkCommandBuffer command_buffer, TOS_mesh* mesh)
{
	vkCmdPushConstants(command_buffer, pipeline.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TOS_push_constant), &push_constant);
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &mesh->vertex_buffer, &offset);
	vkCmdBindIndexBuffer(command_buffer, mesh->index_buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline_layout, 0, 1, &descriptor_pipeline.sets[pipeline.frame_idx], 0, nullptr);
	vkCmdDrawIndexed(command_buffer, (uint32_t) mesh->indices.size(), 1, 0, 0, 0);
}

void record_render_commands(uint32_t image_index)
{
	VkCommandBuffer command_buffer = pipeline.render_command_buffers[pipeline.frame_idx];

	begin_frame(command_buffer, image_index);

	push_constant.M = model.M();
	push_constant.texture_idx = 0;
	push_constant.wireframe = wireframe_timeline.normalized();
	draw_mesh(command_buffer, &mesh);
	
	if(show_aabb)
	{
		push_constant.texture_idx = 1;
		push_constant.wireframe = 1;
		vkCmdPushConstants(command_buffer, pipeline.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TOS_push_constant), &push_constant);
		draw_mesh(command_buffer, &aabb);
	}

	if(show_gui)
	{
		TOS_gui_begin_frame(command_buffer);
		TOS_gui_begin_overlay();
		ImGui::Text("[SHIFT]+[TAB] to toggle overlay");
		ImGui::Text("FPS: %d", TOS_get_FPS());
		ImGui::Checkbox("Wireframe", &wireframe_latch.state);
		TOS_gui_end_overlay();
		TOS_gui_end_frame();
	}

	end_frame(command_buffer);
}

void render_tick()
{
	vkWaitForFences(device.logical, 1, &pipeline.frame_fences[pipeline.frame_idx], VK_TRUE, UINT64_MAX);
	uint32_t image_idx;
	VkResult result = vkAcquireNextImageKHR(device.logical, swapchain.handle, UINT64_MAX, pipeline.image_semaphores[pipeline.frame_idx], VK_NULL_HANDLE, &image_idx);
	if(result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		TOS_rebuild_swapchain(&context, &device, &swapchain);
		return;
	}
	else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("TOS_draw_frame: failed to acquire image from swapchain");
	}
	vkResetFences(device.logical, 1, &pipeline.frame_fences[pipeline.frame_idx]);
	
	vkResetCommandBuffer(pipeline.render_command_buffers[pipeline.frame_idx], 0);
	record_render_commands(image_idx);
	
	VkSubmitInfo submission {};
	submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submission.commandBufferCount = 1;
	submission.pCommandBuffers = &pipeline.render_command_buffers[pipeline.frame_idx];
	
	submission.waitSemaphoreCount = 1;
	submission.pWaitSemaphores = &pipeline.image_semaphores[pipeline.frame_idx];
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submission.pWaitDstStageMask = wait_stages;
	
	submission.signalSemaphoreCount = 1;
	submission.pSignalSemaphores = &pipeline.render_semaphores[pipeline.frame_idx];
	
	result = vkQueueSubmit(device.queues.graphics, 1, &submission, pipeline.frame_fences[pipeline.frame_idx]);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_draw_frame: failed to submit command buffer");
	
	VkPresentInfoKHR presentation {};
	presentation.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentation.waitSemaphoreCount = 1;
	presentation.pWaitSemaphores = &pipeline.render_semaphores[pipeline.frame_idx];
	
	presentation.swapchainCount = 1;
	presentation.pSwapchains = &swapchain.handle;
	presentation.pImageIndices = &image_idx;
	presentation.pResults = nullptr;
	
	result = vkQueuePresentKHR(device.queues.present, &presentation);
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || (context.window_flags & TOS_WINDOW_FLAG_RESIZE))
	{
		TOS_rebuild_swapchain(&context, &device, &swapchain);
		context.window_flags &= ~TOS_WINDOW_FLAG_RESIZE;
	}
	else if(result != VK_SUCCESS)
	{
		throw std::runtime_error("TOS_draw_frame: failed to present swapchain image");
	}
	
	pipeline.frame_idx = (pipeline.frame_idx + 1) % MAX_CONCURRENT_FRAMES;
}

int main(int argc, const char * argv[])
{	
	try
	{
		TOS_create_context(&context, 1280, 720, "Renderer");
		TOS_create_device(&context, &device);
		TOS_create_swapchain(&context, &device, &swapchain);

		for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
			TOS_create_uniform_buffer(&device, &uniform_buffers[i]);
		TOS_load_mesh(&device, &mesh, "assets/meshes/viking_room.obj");
		TOS_AABB_mesh(&device, &aabb, mesh.min, mesh.max);
		TOS_load_texture(&device, &textures[0], "assets/textures/viking_room.ppm");
		TOS_load_texture(&device, &textures[1], "assets/textures/red.ppm");

		TOS_create_descriptor_pipeline(&descriptor_pipeline, MAX_CONCURRENT_FRAMES);
		TOS_register_descriptor_binding(&descriptor_pipeline, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
		TOS_register_descriptor_binding(&descriptor_pipeline, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		TOS_create_descriptor_layout(&device, &descriptor_pipeline);
		TOS_create_descriptor_pool(&device, &descriptor_pipeline);
		TOS_allocate_descriptor_sets(&device, &descriptor_pipeline);
		for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
		{
			TOS_update_uniform_buffer_descriptor(&device, &descriptor_pipeline, 0, i, &uniform_buffers[i]);
			TOS_update_image_sampler_descriptor(&device, &descriptor_pipeline, 1, i, textures);
		}

		TOS_create_pipeline(&device, &swapchain, &descriptor_pipeline, &pipeline);
		TOS_create_gui_context(&context, &device, &swapchain);

		logic_init();

		while(!glfwWindowShouldClose(context.window_handle))
		{
			glfwPollEvents();
				
			logic_tick();
			render_tick();
		}
		vkDeviceWaitIdle(device.logical);

		TOS_destroy_gui_context();

		TOS_destroy_pipeline(&device, &pipeline);
		TOS_destroy_descriptor_pipeline(&device, &descriptor_pipeline);
		TOS_destroy_texture(&device, &textures[1]);
		TOS_destroy_texture(&device, &textures[0]);
		TOS_destroy_mesh(&device, &aabb);
		TOS_destroy_mesh(&device, &mesh);
		for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
			TOS_destroy_uniform_buffer(&device, &uniform_buffers[i]);
		TOS_destroy_swapchain(&device, &swapchain);
		TOS_destroy_device(&context, &device);
		TOS_destroy_context(&context);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
