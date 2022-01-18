cmake_minimum_required(VERSION 3.16)

set(PROJECTNAME "Math")

file(GLOB_RECURSE inc ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE inl ${CMAKE_CURRENT_SOURCE_DIR}/*.inl)
file(GLOB_RECURSE natvis ${CMAKE_CURRENT_SOURCE_DIR}/*.natvis)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inl})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${natvis})

add_library(${PROJECTNAME} INTERFACE
	${inc}
	${inl}
	${natvis})

set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 23)
