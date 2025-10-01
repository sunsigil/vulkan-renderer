#include "shader.h"

#include "memory.h"

VkShaderModule TOS_load_shader(TOS_device* device, const char* path)
{
	size_t file_size;
	char* file_data = (char*) TOS_map_file(path, TOS_FILE_MAP_PRIVATE, &file_size);

	VkShaderModuleCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = file_size;
	create_info.pCode = (uint32_t*) file_data;
	
	VkShaderModule shader_module;
	VkResult result = vkCreateShaderModule(device->logical, &create_info, nullptr, &shader_module);
	if(result != VK_SUCCESS)
		throw std::runtime_error("TOS_load_shader: failed to create shader module");
		
	TOS_unmap_file(file_data, file_size);
	return shader_module;
}