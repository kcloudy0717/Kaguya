cmake_minimum_required(VERSION 3.16)

set(PROJECTNAME "System")

file(GLOB_RECURSE inc ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
file(GLOB_RECURSE natvis ${CMAKE_CURRENT_SOURCE_DIR}/*.natvis)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${natvis})

add_library(
	${PROJECTNAME} STATIC
	${inc}
	${src}
	${natvis})

if (MSVC)
	target_compile_options(${PROJECTNAME} PRIVATE "/W3") # Warning level 3
	target_compile_options(${PROJECTNAME} PRIVATE "/MP") # Multi-processor compilation
endif()

set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 23)

target_include_directories(${PROJECTNAME} PUBLIC "${SYSTEM_DIR}")
