#version 450
#extension GL_EXT_fragment_shader_barycentric  : require

layout(binding = 1) uniform sampler2D textures[8];

layout(location = 0) in vec2 in_uv;
layout(location = 1) in float in_wireframe;
layout(location = 2) flat in int in_texture_idx;

layout(location = 0) out vec4 out_colour;

float wireframe(in float thickness, in float falloff)
{
	const vec3 bary = gl_BaryCoordEXT;

	const vec3 dbx = dFdxFine(bary);
	const vec3 dby = dFdyFine(bary);
	const vec3 db  = sqrt(dbx*dbx + dby*dby);

	const vec3 remapped = smoothstep(db * thickness, db * (thickness + falloff), bary);
	const float nearest = min(min(remapped.x, remapped.y), remapped.z);

	return 1.0-nearest;
}

void main()
{
	float contrib = mix(1.0, wireframe(1.0, 0.0), in_wireframe);
	out_colour = texture(textures[in_texture_idx], in_uv) * contrib;
}
