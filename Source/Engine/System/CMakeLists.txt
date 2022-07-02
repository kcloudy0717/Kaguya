cmake_minimum_required(VERSION 3.16)

set(PROJECTNAME "System")

file(GLOB_RECURSE inc ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE natvis ${CMAKE_CURRENT_SOURCE_DIR}/*.natvis)

set(cityhash
	${DEPDIR}/google/cityhash/city.h
	${DEPDIR}/google/cityhash/city.cc)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${natvis})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${cityhash})

add_library(${PROJECTNAME} STATIC
	${inc}
	${src}
	${natvis}

	${cityhash})

if (MSVC)
	target_compile_options(${PROJECTNAME} PRIVATE "/W3") # warning level 3
	target_compile_options(${PROJECTNAME} PRIVATE "/MP") # Multi-processor compilation
endif()

set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 23)

target_include_directories(${PROJECTNAME} PUBLIC "${SYSTEM_DIR}")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/spdlog/include")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/robin-hood-hashing/src/include")

target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/google/cityhash")
