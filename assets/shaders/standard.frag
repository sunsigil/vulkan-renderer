#version 450

layout(binding = 1) uniform sampler2D tex;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_colour;

void main()
{
	out_colour = texture(tex, in_uv);
}
