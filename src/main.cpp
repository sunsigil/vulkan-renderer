#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include "context.h"
#include "device.h"
#include "swapchain.h"
#include "pipeline.h"
#include "input.h"
#include <chrono>
typedef std::chrono::time_point<std::chrono::high_resolution_clock> timepoint;

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

TOS_context context;
TOS_device device;
TOS_swapchain swapchain;
TOS_descriptor_pipeline descriptor_pipeline;
TOS_uniform_buffer uniform_buffers[MAX_CONCURRENT_FRAMES];
TOS_mesh mesh;
TOS_texture texture;
TOS_pipeline pipeline;

void update_uniforms()
{
	static timepoint start_time = std::chrono::high_resolution_clock::now();
	timepoint current_time = std::chrono::high_resolution_clock::now();
	float t = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
	
	TOS_UBO ubo {};
	ubo.M = glm::rotate(glm::mat4(1.0f), t * glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.V = glm::lookAt(glm::vec3(2, 2, 2), glm::vec3(0, 0, 0), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.P = glm::perspective(glm::radians(45.0f), (float) swapchain.extent.width / (float) swapchain.extent.height, 0.01f, 100.0f);
	ubo.P[1][1] *= -1.0f;
	
	memcpy(uniform_buffers[pipeline.frame_idx].pointer, &ubo, sizeof(ubo));
}

void record_render_commands(uint32_t image_index)
{
	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = nullptr;

	VkCommandBuffer command_buffer = pipeline.render_command_buffers[pipeline.frame_idx];
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
	
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
	
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
	
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &mesh.vertex_buffer, &offset);
	vkCmdBindIndexBuffer(command_buffer, mesh.index_buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline_layout, 0, 1, &descriptor_pipeline.sets[pipeline.frame_idx], 0, nullptr);
	
	vkCmdDrawIndexed(command_buffer, (uint32_t) mesh.indices.size(), 1, 0, 0, 0);

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), pipeline.render_command_buffers[pipeline.frame_idx]);

	vkCmdEndRenderPass(command_buffer);
	result = vkEndCommandBuffer(command_buffer);
	if(result != VK_SUCCESS)
		throw std::runtime_error("[ERROR] failed to finish recording render command buffer");
}

int main(int argc, const char * argv[])
{	
	try
	{
		TOS_create_context(&context, 1280, 720, "Vulkan App");
		TOS_create_device(&context, &device);
		TOS_create_swapchain(&context, &device, &swapchain);

		for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
			TOS_create_uniform_buffer(&device, &uniform_buffers[i]);
		TOS_load_mesh(&device, &mesh, "assets/meshes/viking_room.obj");
		TOS_load_texture(&device, &texture, "assets/textures/viking_room.ppm");

		TOS_create_descriptor_pipeline(&descriptor_pipeline, MAX_CONCURRENT_FRAMES);
		TOS_register_descriptor_binding(&descriptor_pipeline, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
		TOS_register_descriptor_binding(&descriptor_pipeline, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		TOS_create_descriptor_layout(&device, &descriptor_pipeline);
		TOS_create_descriptor_pool(&device, &descriptor_pipeline);
		TOS_allocate_descriptor_sets(&device, &descriptor_pipeline);
		for(int i = 0; i < MAX_CONCURRENT_FRAMES; i++)
		{
			TOS_update_uniform_buffer_descriptor(&device, &descriptor_pipeline, 0, i, &uniform_buffers[i]);
			TOS_update_image_sampler_descriptor(&device, &descriptor_pipeline, 1, i, &texture);
		}

		TOS_create_pipeline(&device, &swapchain, &descriptor_pipeline, &pipeline);

		TOS_create_input_context(&context);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		[[maybe_unused]]
		ImGuiIO& io = ImGui::GetIO();
		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForVulkan(context.window_handle, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = context.instance;
		init_info.PhysicalDevice = device.physical;
		init_info.Device = device.logical;
		init_info.QueueFamily = TOS_query_queue_families(&context, device.physical).graphics.value();
		init_info.Queue = device.queues.graphics;
		init_info.PipelineCache = VK_NULL_HANDLE;

		VkDescriptorPool imgui_descriptor_pool;
		VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 0;
        for (VkDescriptorPoolSize& pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;
        pool_info.poolSizeCount = (uint32_t) IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        VkResult result = vkCreateDescriptorPool(device.logical, &pool_info, nullptr, &imgui_descriptor_pool);
		if(result != VK_SUCCESS)
			throw std::runtime_error("main: failed to create imgui descriptor pool");
		init_info.DescriptorPool = imgui_descriptor_pool;

		init_info.Allocator = nullptr;
		init_info.MinImageCount = 2;
		init_info.ImageCount = swapchain.images.size();
		init_info.CheckVkResultFn = nullptr;

		init_info.PipelineInfoMain.RenderPass = swapchain.render_pass;
		init_info.PipelineInfoMain.Subpass = 0;
		init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info);

		while(!glfwWindowShouldClose(context.window_handle))
		{
			glfwPollEvents();
			TOS_input_tick();

			vkWaitForFences(device.logical, 1, &pipeline.frame_fences[pipeline.frame_idx], VK_TRUE, UINT64_MAX);
		
			uint32_t image_idx;
			VkResult result = vkAcquireNextImageKHR(device.logical, swapchain.handle, UINT64_MAX, pipeline.image_semaphores[pipeline.frame_idx], VK_NULL_HANDLE, &image_idx);
			if(result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				TOS_rebuild_swapchain(&context, &device, &swapchain);
				continue;
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
			
			update_uniforms();
			
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
		vkDeviceWaitIdle(device.logical);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		vkDestroyDescriptorPool(device.logical, imgui_descriptor_pool, nullptr);

		TOS_destroy_pipeline(&device, &pipeline);
		TOS_destroy_descriptor_pipeline(&device, &descriptor_pipeline);
		TOS_destroy_texture(&device, &texture);
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
