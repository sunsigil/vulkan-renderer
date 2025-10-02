#version 450

layout(binding = 1) uniform sampler2D tex;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_colour;

#extension GL_EXT_fragment_shader_barycentric  : require

float WireFrame(in float Thickness, in float Falloff)
{
	const vec3 BaryCoord = gl_BaryCoordEXT;

	const vec3 dBaryCoordX = dFdxFine(BaryCoord);
	const vec3 dBaryCoordY = dFdyFine(BaryCoord);
	const vec3 dBaryCoord  = sqrt(dBaryCoordX*dBaryCoordX + dBaryCoordY*dBaryCoordY);

	const vec3 dFalloff   = dBaryCoord * Falloff;
	const vec3 dThickness = dBaryCoord * Thickness;

	const vec3 Remap = smoothstep(dThickness, dThickness + dFalloff, BaryCoord);
	const float ClosestEdge = min(min(Remap.x, Remap.y), Remap.z);

	return 1.0 - ClosestEdge;
}

void main()
{
	out_colour = vec4(WireFrame(0.5, 0.5)); //texture(tex, in_uv);
}
