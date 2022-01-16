cmake_minimum_required(VERSION 3.16)

set(PROJECTNAME "System")

file(GLOB_RECURSE inc ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

set(cityhash
	${DEPDIR}/google/cityhash/city.h
	${DEPDIR}/google/cityhash/city.cc)
set(spdlog
	${DEPDIR}/spdlog/include/spdlog/spdlog.h)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${cityhash})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${spdlog})

add_library(${PROJECTNAME} STATIC
	${inc}
	${src}

	${cityhash}
	${spdlog})
target_precompile_headers(${PROJECTNAME} PRIVATE pch.h)

if (MSVC)
	target_compile_options(${PROJECTNAME} PRIVATE "/W3") # warning level 3
	target_compile_options(${PROJECTNAME} PRIVATE "/MP") # Multi-processor compilation
endif()

set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 23)

target_include_directories(${PROJECTNAME} PRIVATE "${SYSTEM_DIR}")
target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/wil/include")
target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/google/cityhash")
target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/spdlog/include")
