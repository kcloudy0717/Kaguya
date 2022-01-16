cmake_minimum_required(VERSION 3.16)

set(PROJECTNAME "Math")

file(GLOB_RECURSE inc ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})

add_library(${PROJECTNAME} STATIC
	${inc}
	${src})

if (MSVC)
	target_compile_options(${PROJECTNAME} PRIVATE "/W3") # warning level 3
	target_compile_options(${PROJECTNAME} PRIVATE "/MP") # Multi-processor compilation
endif()

set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 23)
