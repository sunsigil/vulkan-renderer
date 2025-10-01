cmake_minimum_required(VERSION 3.20.0)

project(main)
add_executable(main)

add_subdirectory(assets/shaders)
add_dependencies(main shaders)

find_package(glfw3 3.3 REQUIRED)
find_package(Vulkan REQUIRED)

target_include_directories(main PRIVATE
	${Vulkan_INCLUDE_DIRS}
	src)
	
target_sources(main PRIVATE
	src/context.cpp
	src/device.cpp
	src/memory.cpp
	src/pipeline.cpp
	src/shader.cpp
	src/swapchain.cpp
	src/textures.cpp
	src/vertices.cpp

	src/ppm/ppm.cpp
	src/obj/obj.cpp
	
	src/main.cpp)
target_link_libraries(main m glfw Vulkan::Vulkan)
target_compile_options(main PRIVATE -Wall -g -std=c++17)
target_compile_definitions(main PRIVATE GLFW_INCLUDE_VULKAN GLM_ENABLE_EXPERIMENTAL GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS)