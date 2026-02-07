#include "gizmos.h"

#include "vertices.h"
#include "input.h"
#include "geometry.h"
#include "camera.h"
#include "draw.h"
#include <iostream>
#include "glm/gtx/string_cast.hpp"

static TOS_device* device;

static TOS_mesh transform_gizmo_meshes[3];
static TOS_camera* camera;

void TOS_create_gizmo_context(TOS_device* _device, TOS_camera* _camera)
{
	device = _device;

	TOS_load_mesh(device, &transform_gizmo_meshes[0], "assets/meshes/gizmo_translate.obj");
	TOS_load_mesh(device, &transform_gizmo_meshes[1], "assets/meshes/gizmo_rotate.obj");
	TOS_load_mesh(device, &transform_gizmo_meshes[2], "assets/meshes/gizmo_scale.obj");

	camera = _camera;
}

void TOS_destroy_gizmo_context()
{
	TOS_destroy_mesh(device, &transform_gizmo_meshes[0]);
	TOS_destroy_mesh(device, &transform_gizmo_meshes[1]);
	TOS_destroy_mesh(device, &transform_gizmo_meshes[2]);
}

static TOS_transform* transform;
static TOS_transform_op transform_op;

void TOS_set_transform_gizmo_target(TOS_transform* _transform)
{
	transform = _transform;
}

bool TOS_is_transform_gizmo_active()
{
	return transform != nullptr;
}

void TOS_set_transform_gizmo_op(TOS_transform_op op)
{
	transform_op = op;
}

static TOS_segment segments[3];
static glm::vec3 contacts[3];

static std::optional<TOS_axis> axis;
static glm::vec3 drag_now;
static glm::vec3 drag_then;
static glm::vec3 drag_delta;

void get_axis()
{
	axis.reset();

	segments[0] = TOS_segment { .a = transform->position, .b = transform->position + glm::vec3(1,0,0) };
	segments[1] = TOS_segment { .a = transform->position, .b = transform->position + glm::vec3(0,1,0) };
	segments[2] = TOS_segment { .a = transform->position, .b = transform->position + glm::vec3(0,0,1) };

	glm::vec2 mouse = TOS_mouse_position(true);
	TOS_ray ray = camera->viewport_ray(mouse.x, mouse.y);
	glm::vec3 distances
	(
		TOS_ray_segment_nearest(ray, segments[0], nullptr, &contacts[0]),
		TOS_ray_segment_nearest(ray, segments[1], nullptr, &contacts[1]),
		TOS_ray_segment_nearest(ray, segments[2], nullptr, &contacts[2])
	);

	if(distances.x > 0.05f && distances.y > 0.05f && distances.z > 0.05f)
		return;
	if(distances.z <= distances.x && distances.z <= distances.y)
		axis = TOS_AXIS_Z;
	if(distances.y <= distances.z && distances.y <= distances.x)
		axis = TOS_AXIS_Y;
	if(distances.x <= distances.y && distances.x <= distances.z)
		axis = TOS_AXIS_X;
	drag_then = drag_now = contacts[(int) axis.value()];
}

void get_drag_delta()
{
	if(axis.has_value())
	{
		int comp = (int) axis.value();

		glm::vec2 mouse = TOS_mouse_position(true);
		TOS_ray ray = camera->viewport_ray(mouse.x, mouse.y);
		TOS_ray_segment_nearest(ray, segments[comp], nullptr, &contacts[comp]),

		drag_now = contacts[comp];
		drag_delta = drag_now - drag_then;
		drag_then = drag_now;
	}
}

void TOS_tick_transform_gizmo()
{	
	if(TOS_mouse_pressed())
	{
		get_axis();
	}
	else if(TOS_mouse_down() && axis.has_value())
	{
		get_drag_delta();
		switch(transform_op)
		{
			case TOS_TRANSFORM_OP_TRANSLATE:
				transform->position += drag_delta;
			break;
			case TOS_TRANSFORM_OP_ROTATE:
				transform->orientation += drag_delta;
			break;
			case TOS_TRANSFORM_OP_SCALE:
				transform->scale += drag_delta;
			break;
		}
	}
}

void TOS_draw_transform_gizmo()
{
	TOS_clear_depth_buffer();

	glm::vec3 scale = transform->scale;
	transform->scale = glm::vec3(1, 1, 1);
	glm::mat4 M = transform->M();
	transform->scale = scale;

	TOS_push_constants constants =
	{
		.M = M,
		.texture_idx = 2,
		.wireframe = 0
	};
	TOS_set_push_constants(&constants);
	TOS_draw_mesh(&transform_gizmo_meshes[(int) transform_op]);
}