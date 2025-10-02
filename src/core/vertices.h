#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/hash.hpp>
#include "device.h"

struct TOS_vertex
{
	glm::vec3 position;
	glm::vec2 uv;
	
	bool operator==(const TOS_vertex& other) const;
};

namespace std
{
	template<> struct hash<TOS_vertex>
	{
		size_t operator()(TOS_vertex const& vertex) const
		{
			return
			(hash<glm::vec3>()(vertex.position)) ^
			(hash<glm::vec2>()(vertex.uv));
		}
	};
}

VkVertexInputBindingDescription TOS_get_vertex_binding_description();
std::array<VkVertexInputAttributeDescription, 2> TOS_get_vertex_attribute_descriptions();

struct TOS_mesh
{
	std::vector<TOS_vertex> vertices;
	VkBuffer vertex_buffer = VK_NULL_HANDLE;
	VkDeviceMemory vertex_memory = VK_NULL_HANDLE;

	std::vector<uint32_t> indices;
	VkBuffer index_buffer = VK_NULL_HANDLE;
	VkDeviceMemory index_memory = VK_NULL_HANDLE;
};

void TOS_create_mesh(TOS_device* device, TOS_mesh* mesh, std::vector<TOS_vertex> vertices, std::vector<uint32_t> indices);
void TOS_load_mesh(TOS_device* device, TOS_mesh* mesh, const char* path);
void TOS_destroy_mesh(TOS_device* device, TOS_mesh* mesh);