#include "vertices.h"

#include "obj/obj.h"
#include "memory.h"

bool TOS_vertex::operator==(const TOS_vertex& other) const
{
	return
	position == other.position &&
	uv == other.uv;
}

VkVertexInputBindingDescription TOS_get_vertex_binding_description()
{
	VkVertexInputBindingDescription description {};
	description.binding = 0;
	description.stride = sizeof(TOS_vertex);
	description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return description;
}

std::array<VkVertexInputAttributeDescription, 2> TOS_get_vertex_attribute_descriptions()
{
	std::array<VkVertexInputAttributeDescription, 2> descriptions {};
	descriptions[0] =
	{
		.binding = 0,
		.location = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(TOS_vertex, position)
	};
	descriptions[1] =
	{
		.binding = 0,
		.location = 1,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = offsetof(TOS_vertex, uv)
	};
	return descriptions;
}

void create_vertex_buffer(TOS_device* device, TOS_mesh* mesh)
{
	VkDeviceSize buffer_size = sizeof(TOS_vertex) * mesh->vertices.size();
	
	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
	TOS_create_buffer
	(
		device, buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer,
		staging_memory
	);
	
	void* data;
	vkMapMemory(device->logical, staging_memory, 0, buffer_size, 0, &data);
	memcpy(data, mesh->vertices.data(), buffer_size);
	vkUnmapMemory(device->logical, staging_memory);
	
	TOS_create_buffer
	(
		device, buffer_size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mesh->vertex_buffer,
		mesh->vertex_memory
	);
	
	TOS_copy_buffer(device, staging_buffer, mesh->vertex_buffer, buffer_size);
	vkDestroyBuffer(device->logical, staging_buffer, nullptr);
	vkFreeMemory(device->logical, staging_memory, nullptr);
}

void create_index_buffer(TOS_device* device, TOS_mesh* mesh)
{
	VkDeviceSize buffer_size = sizeof(uint32_t) * mesh->indices.size();
	
	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
	TOS_create_buffer
	(
		device, buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer,
		staging_memory
	);
	
	void* data;
	vkMapMemory(device->logical, staging_memory, 0, buffer_size, 0, &data);
	memcpy(data, mesh->indices.data(), buffer_size);
	vkUnmapMemory(device->logical, staging_memory);
	
	TOS_create_buffer
	(
		device, buffer_size,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mesh->index_buffer,
		mesh->index_memory
	);
	
	TOS_copy_buffer(device, staging_buffer, mesh->index_buffer, buffer_size);
	vkFreeMemory(device->logical, staging_memory, nullptr);
	vkDestroyBuffer(device->logical, staging_buffer, nullptr);
}

void TOS_create_mesh(TOS_device* device, TOS_mesh* mesh, std::vector<TOS_vertex> vertices, std::vector<uint32_t> indices)
{
	mesh->vertices = vertices;
	mesh->indices = indices;
	create_vertex_buffer(device, mesh);
	create_index_buffer(device, mesh);
}

void TOS_load_mesh(TOS_device* device, TOS_mesh* mesh, const char* path)
{
	TOS_OBJ obj;
	TOS_OBJ_load(&obj, path);
	
	std::vector<TOS_vertex> vertices;
	std::vector<uint32_t> indices;
	std::unordered_map<TOS_vertex, uint32_t> unique_vertices;

	for(int f_idx = 0; f_idx < obj.f.size(); f_idx += 9)
	{
		for(int pt_idx = 0; pt_idx < 9; pt_idx += 3)
		{
			int v_idx = (obj.f[f_idx+pt_idx+0]-1)*3;
			int vt_idx = (obj.f[f_idx+pt_idx+1]-1)*2;
			int vn_idx = (obj.f[f_idx+pt_idx+2]-1)*3;

			glm::vec3 v =
			{
				obj.v[v_idx+0],
				obj.v[v_idx+1],
				obj.v[v_idx+2]
			};
			glm::vec2 vt = vt_idx >= 0 ?
			(glm::vec2) {
				obj.vt[vt_idx+0],
				1.0f - obj.vt[vt_idx+1],
			} :
			(glm::vec2) {
				0, 0
			};

			TOS_vertex vertex =
			{
				.position = v,
				.uv = vt
			};
			
			if(unique_vertices.count(vertex) == 0)
			{
				unique_vertices[vertex] = (uint32_t) vertices.size();
				vertices.push_back(vertex);
			}
			indices.push_back(unique_vertices[vertex]);
		}
	}

	TOS_create_mesh(device, mesh, vertices, indices);
}

void TOS_destroy_mesh(TOS_device* device, TOS_mesh* mesh)
{
	vkFreeMemory(device->logical, mesh->index_memory, nullptr);
	vkDestroyBuffer(device->logical, mesh->index_buffer, nullptr);
	vkFreeMemory(device->logical, mesh->vertex_memory, nullptr);
	vkDestroyBuffer(device->logical, mesh->vertex_buffer, nullptr);
}