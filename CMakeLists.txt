cmake_minimum_required(VERSION 3.20.0)

project(main)
add_executable(main)

add_subdirectory(assets/shaders)
add_dependencies(main shaders)

find_package(glfw3 3.3 REQUIRED)
find_package(Vulkan REQUIRED)

target_include_directories(main PRIVATE
	${Vulkan_INCLUDE_DIRS}
	src/core
	src)
	
target_sources(main PRIVATE
	src/core/context.cpp
	src/core/device.cpp
	src/core/memory.cpp
	src/core/pipeline.cpp
	src/core/shader.cpp
	src/core/swapchain.cpp
	src/core/textures.cpp
	src/core/vertices.cpp

	src/ppm/ppm.cpp
	src/obj/obj.cpp
	
	src/input.cpp
	src/gui.cpp
	src/timing.cpp
	src/machines.cpp

	src/imgui/imgui_demo.cpp
	src/imgui/imgui_draw.cpp
	src/imgui/imgui_impl_glfw.cpp
	src/imgui/imgui_impl_vulkan.cpp
	src/imgui/imgui_tables.cpp
	src/imgui/imgui_widgets.cpp
	src/imgui/imgui.cpp

	src/main.cpp)
target_link_libraries(main m glfw Vulkan::Vulkan)
target_compile_options(main PRIVATE -Wall -g -std=c++17)
target_compile_definitions(main PRIVATE GLFW_INCLUDE_VULKAN GLM_ENABLE_EXPERIMENTAL GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS)