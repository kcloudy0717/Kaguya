cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE inc ${CMAKE_CURRENT_SOURCE_DIR}/*.hlsli)
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/*.hlsl)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})
set_source_files_properties(${src} PROPERTIES VS_TOOL_OVERRIDE None)

add_custom_target(KaguyaShaders ALL SOURCES ${inc} ${src} DEPENDS ${inc} ${src})
add_custom_command(
	TARGET KaguyaShaders
	COMMAND ${CMAKE_COMMAND}
	-E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>/Shaders)

set_target_properties(KaguyaShaders PROPERTIES FOLDER "Apps")
