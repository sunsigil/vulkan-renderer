cmake_minimum_required(VERSION 3.20.0)
project(shaders)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/standard.vert ${CMAKE_CURRENT_SOURCE_DIR}/standard.frag)
set(OUTPUTS "")

foreach(SOURCE ${SOURCES})
	get_filename_component(NAME ${SOURCE} NAME)
	set(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.spv)
	list(APPEND OUTPUTS ${OUTPUT})

	add_custom_command(
		OUTPUT ${OUTPUT}
		COMMAND glslc ${SOURCE} -o ${OUTPUT}
		DEPENDS ${SOURCE}
	)
endforeach()

add_custom_target(
	shaders
	DEPENDS ${OUTPUTS}
)
