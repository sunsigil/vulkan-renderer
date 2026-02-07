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
#include "shader_common.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include "glm/gtx/string_cast.hpp"

static TOS_context context;
static TOS_device device;
static TOS_swapchain swapchain;

static TOS_mesh sponza_mesh;
static TOS_mesh sphere_mesh;
static TOS_mesh aabb_mesh;
static TOS_mesh screen_mesh;

static TOS_pipeline pipeline;

static TOS_camera camera;
static TOS_UBO uniforms;
static TOS_push_constants push_constant;
static TOS_transform model;

static TOS_latch gui_latch(false);
static TOS_latch wireframe_latch(false);
static TOS_timeline wireframe_timeline(0.5, true);
static bool selected;
static int transform_op = TOS_TRANSFORM_OP_TRANSLATE;

static TOS_image rt_frame;
static TOS_latch rt_latch(false);

void logic_init()
{
	TOS_create_timing_context();
	TOS_create_input_context(&context);

	camera = TOS_camera
	(
		glm::vec3(0, 0, 0), glm::vec3(0),
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
		gui_latch.flip();
		TOS_toggle_cursor(gui_latch.state);
	}

	if(!gui_latch.state)
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
			TOS_AABB aabb = TOS_AABB::min_max(sphere_mesh.min, sphere_mesh.max);

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

	if(gui_latch.flipped())
	{
		rt_latch.flip();
	}

	if(wireframe_latch.flipped())
	{
		wireframe_timeline.reset();
		wireframe_timeline.resume();
		wireframe_timeline.reverse();
	}

	if(rt_latch.state && rt_latch.flipped())
	{
		TOS_sphere sphere = TOS_sphere::center_radius(model.position, 0.5f);
		for(int y = 0; y < rt_frame.height; y++)
		{
			float v = y / (float) (rt_frame.height-1);
			for(int x = 0; x < rt_frame.width; x++)
			{
				float u = x / (float) (rt_frame.width-1);
				TOS_ray ray = camera.viewport_ray(u, v);
				std::optional<TOS_raycast_hit> hit = TOS_ray_sphere_intersect(ray, sphere);
				if(hit.has_value())
				{
					glm::vec3 n = hit.value().normal;
					glm::vec3 l = glm::vec3(-1, 1, -1);
					float D = glm::dot(glm::normalize(n), glm::normalize(l));
					TOS_set_pixel
					(
						&rt_frame, x, y,
						D * 255, 0,
						0, 255
					);
				}
				else
				{
					TOS_set_pixel
					(
						&rt_frame, x, y, 0, 0, 0, 0
					);
				}
			}
		}
		TOS_update_texture(&device, &textures[3], &rt_frame);
	}

	// POST-TICKS

	gui_latch.tick();
	wireframe_timeline.tick();
	wireframe_latch.tick();
	rt_latch.tick();
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

	push_constant.flags = 0;

	push_constant.M = model.M();
	push_constant.texture_idx = 0;
	push_constant.wireframe = wireframe_timeline.normalized();
	TOS_set_push_constants(&push_constant);
	TOS_draw_mesh(&sponza_mesh);

	if(!rt_latch.state)
	{
		push_constant.M = model.M();
		push_constant.texture_idx = 1;
		push_constant.wireframe = wireframe_timeline.normalized();
		TOS_set_push_constants(&push_constant);
		TOS_draw_mesh(&sphere_mesh);

		if(TOS_is_transform_gizmo_active())
		{
			TOS_draw_transform_gizmo();
			
			push_constant.wireframe = 1;
			push_constant.texture_idx = 1;
			TOS_set_push_constants(&push_constant);
			TOS_draw_mesh(&aabb_mesh);
		}
	}

	if(gui_latch.state)
	{
		if(rt_latch.state)
		{
			push_constant.flags = TOS_SHADER_FLAG_NDC_GEOMETRY;
			push_constant.texture_idx = 3;
			push_constant.wireframe = 0;
			TOS_set_push_constants(&push_constant);
			TOS_draw_mesh(&screen_mesh);
		}

		TOS_gui_begin_frame();
		TOS_gui_begin_overlay();
		ImGui::Text("[SHIFT]+[TAB] to toggle overlay");
		ImGui::Text("FPS: %d", TOS_get_FPS());
		ImGui::Checkbox("Raytracing", &rt_latch.state);
		if(!rt_latch.state)
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

		logic_init();

		TOS_create_image(&rt_frame, context.window_width, context.window_height);
		TOS_create_texture(&device, &textures[3], &rt_frame);

		TOS_load_texture(&device, &textures[0], "assets/textures/sponza/spnza_bricks_a_diff.png");
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

		TOS_load_mesh(&device, &sponza_mesh, "assets/meshes/sponza.obj");
		TOS_load_mesh(&device, &sphere_mesh, "assets/meshes/sphere.obj");
		TOS_AABB_mesh(&device, &aabb_mesh, sphere_mesh.min, sphere_mesh.max);
		TOS_screen_mesh(&device, &screen_mesh);

		TOS_create_gui_context(&context, &device, &swapchain);

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
		
		TOS_destroy_mesh(&device, &sponza_mesh);
		TOS_destroy_mesh(&device, &aabb_mesh);
		TOS_destroy_mesh(&device, &sphere_mesh);
		TOS_destroy_mesh(&device, &screen_mesh);
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
