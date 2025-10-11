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
#include "cowtools.h"
#include "gizmos.h"
#include "draw.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include "glm/gtx/string_cast.hpp"

static TOS_context context;
static TOS_device device;
static TOS_swapchain swapchain;

static TOS_mesh mesh;
static TOS_mesh aabb_mesh;

static TOS_pipeline pipeline;

static TOS_camera camera;
static TOS_UBO uniforms;
static TOS_push_constants push_constant;
static TOS_transform model;

static bool show_gui;
static TOS_latch wireframe_latch(false);
static TOS_timeline wireframe_timeline(0.5, true);
static bool selected;
static int transform_op = TOS_TRANSFORM_OP_TRANSLATE;

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
	else
	{
		if(TOS_mouse_pressed())
		{
			glm::vec2 mouse = TOS_mouse_position(true);
			TOS_ray ray = camera.viewport_ray(mouse.x, mouse.y);
			TOS_AABB aabb = TOS_AABB::min_max(mesh.min, mesh.max);

			std::optional<TOS_raycast_hit> hit = TOS_ray_OBB_intersect(ray, aabb, model.M());
			if(hit.has_value())
				TOS_set_transform_gizmo_target(&model);
			else
				TOS_set_transform_gizmo_target(nullptr);
		}

		if(TOS_is_transform_gizmo_active())
		{
			if(TOS_key_pressed(GLFW_KEY_G))
				TOS_set_transform_gizmo_op(TOS_TRANSFORM_OP_TRANSLATE);
			if(TOS_key_pressed(GLFW_KEY_R))
				TOS_set_transform_gizmo_op(TOS_TRANSFORM_OP_ROTATE);
			if(TOS_key_pressed(GLFW_KEY_S))
				TOS_set_transform_gizmo_op(TOS_TRANSFORM_OP_SCALE);
			
			TOS_tick_transform_gizmo();
		}
	}

	// GUI-CONTROLLED

	if(wireframe_latch.flipped())
	{
		wireframe_timeline.reset();
		wireframe_timeline.resume();
		wireframe_timeline.reverse();
	}

	// POST-TICKS

	wireframe_timeline.tick();
	wireframe_latch.tick();
	camera.tick();
}

void render_tick()
{
	TOS_begin_frame();

	TOS_bind_pipeline(&pipeline);

	uniforms.V = camera.V();
	uniforms.P = camera.P();
	uniforms.P[1][1] *= -1.0f;
	TOS_set_UBO(&uniforms);

	push_constant.M = model.M();
	push_constant.texture_idx = 0;
	push_constant.wireframe = wireframe_timeline.normalized();
	TOS_set_push_constants(&push_constant);
	TOS_draw_mesh(&mesh);

	TOS_draw_transform_gizmo();

	if(TOS_is_transform_gizmo_active())
	{
		push_constant.wireframe = 1;
		push_constant.texture_idx = 1;
		TOS_set_push_constants(&push_constant);
		TOS_draw_mesh(&aabb_mesh);
	}

	if(show_gui)
	{
		TOS_gui_begin_frame();
		TOS_gui_begin_overlay();
		ImGui::Text("[SHIFT]+[TAB] to toggle overlay");
		ImGui::Text("FPS: %d", TOS_get_FPS());
		ImGui::Checkbox("Wireframe", &wireframe_latch.state);
		TOS_gui_end_overlay();
		TOS_gui_end_frame();
	}

	TOS_end_frame();
}

int main(int argc, const char * argv[])
{	
	try
	{
		TOS_create_context(&context, 1280, 720, "Renderer");
		TOS_create_device(&context, &device);
		TOS_create_swapchain(&context, &device, &swapchain);

		TOS_load_texture(&device, &textures[0], "assets/textures/viking_room.png");
		TOS_load_texture(&device, &textures[1], "assets/textures/red.png");
		TOS_load_texture(&device, &textures[2], "assets/textures/gizmo.png");
		TOS_create_drawing_context(&context, &device, &swapchain);

		TOS_pipeline_specification pipeline_spec =
		{
			.vert_path = "build/assets/shaders/standard.vert.spv",
			.frag_path = "build/assets/shaders/standard.frag.spv",
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.polygon_mode = VK_POLYGON_MODE_FILL,
			.depth_compare_op = VK_COMPARE_OP_LESS
		};
		TOS_create_pipeline(&device, &swapchain, &descriptors, pipeline_spec, &pipeline);

		TOS_load_mesh(&device, &mesh, "assets/meshes/viking_room.obj");
		TOS_AABB_mesh(&device, &aabb_mesh, mesh.min, mesh.max);

		TOS_create_gui_context(&context, &device, &swapchain);

		logic_init();

		TOS_create_gizmo_context(&device, &camera);

		while(!glfwWindowShouldClose(context.window_handle))
		{
			glfwPollEvents();
				
			logic_tick();
			render_tick();
		}
		vkDeviceWaitIdle(device.logical);

		TOS_destroy_gizmo_context();

		TOS_destroy_gui_context();

		TOS_destroy_mesh(&device, &aabb_mesh);
		TOS_destroy_mesh(&device, &mesh);
		TOS_destroy_pipeline(&device, &pipeline);
		TOS_destroy_drawing_context();

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
