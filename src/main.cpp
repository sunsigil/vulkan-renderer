#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include "context.h"
#include "device.h"
#include "swapchain.h"
#include "pipeline.h"

int main(int argc, const char * argv[])
{	
	try
	{
		//init
		TOS_context context;
		TOS_create_context(&context, 1280, 720, "Vulkan App");
		TOS_device device;
		TOS_create_device(&context, &device);
		TOS_swapchain swapchain;
		TOS_create_swapchain(&context, &device, &swapchain);
		TOS_mesh mesh;
		TOS_load_mesh(&device, &mesh, "assets/meshes/viking_room.obj");
		TOS_texture texture;
		TOS_load_texture(&device, &texture, "assets/textures/viking_room.ppm");
		TOS_pipeline pipeline;
		TOS_create_pipeline(&device, &swapchain, &pipeline, &mesh, &texture);

		while(!glfwWindowShouldClose(context.window_handle))
		{
			glfwPollEvents();	
			TOS_draw_frame(&context, &device, &swapchain, &pipeline);
		}
		vkDeviceWaitIdle(device.logical);

		TOS_destroy_pipeline(&device, &pipeline);
		TOS_destroy_texture(&device, &texture);
		TOS_destroy_mesh(&device, &mesh);
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
