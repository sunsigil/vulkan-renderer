#version 450
#extension GL_ARB_shading_language_include : require

#include "shader_common.h"

layout(binding = 0) uniform TOS_UBO
{
	mat4 V;
	mat4 P;
} ubo;

layout(push_constant) uniform TOS_push_constant
{
	mat4 M;
	int texture_idx;
	float wireframe;
	uint flags;
} push_constant;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in uint in_material_idx;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out uint out_material_idx;

layout(location = 3) out float out_wireframe;
layout(location = 4) out int out_texture_idx;
layout(location = 5) out uint out_flags;

void main()
{
	mat4 MVP = mat4(1.0);
	if((push_constant.flags & TOS_SHADER_FLAG_NDC_GEOMETRY) < 1)
		MVP = ubo.P * ubo.V * push_constant.M;
	
	out_uv = in_uv;
	out_normal = in_normal;
	out_material_idx = in_material_idx;

	out_wireframe = push_constant.wireframe;
	out_texture_idx = push_constant.texture_idx;
	out_flags = push_constant.flags;

	gl_Position = MVP * vec4(in_position, 1.0);
}
