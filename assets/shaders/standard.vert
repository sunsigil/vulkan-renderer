#version 450

layout(binding = 0) uniform TOS_UBO
{
	mat4 M;
	mat4 V;
	mat4 P;
} ubo;

layout(push_constant) uniform TOS_push_constant
{
	int texture_idx;
	float wireframe;
} push_constant;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out float out_wireframe;
layout(location = 2) out int out_texture_idx;

void main()
{
	mat4 MVP = ubo.P * ubo.V * ubo.M;
	gl_Position = MVP * vec4(in_position, 1.0);
	
	out_uv = in_uv;
	out_wireframe = push_constant.wireframe;
	out_texture_idx = push_constant.texture_idx;
}
