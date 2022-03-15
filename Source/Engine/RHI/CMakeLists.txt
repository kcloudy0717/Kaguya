cmake_minimum_required(VERSION 3.16)

set(PROJECTNAME "RHI")

file(GLOB_RECURSE inc ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${cityhash})

add_library(${PROJECTNAME} STATIC
	${inc}
	${src})

if (MSVC)
	target_compile_options(${PROJECTNAME} PRIVATE "/W3") # warning level 3
	target_compile_options(${PROJECTNAME} PRIVATE "/MP") # Multi-processor compilation
endif()

set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 23)

target_include_directories(${PROJECTNAME} PRIVATE ${RHI_DIR})
target_include_directories(${PROJECTNAME} PRIVATE ${ENGINE_DIR})
target_link_libraries(${PROJECTNAME} PRIVATE System)

# Public include/linking for now until a RHI abstraction is build on top of it
target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/robin-hood-hashing/src/include")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/AgilitySDK/Include")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/WinPixEventRuntime/Include")
target_link_libraries(${PROJECTNAME} PRIVATE ${DEPDIR}/WinPixEventRuntime/Bin/x64/WinPixEventRuntime.lib)
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/dxc")
target_link_libraries(${PROJECTNAME} PRIVATE ${DEPDIR}/dxc/dxcompiler.lib)

# Linking

# DLL
add_custom_command(
	TARGET ${PROJECTNAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy 
		${DEPDIR}/dxc/dxcompiler.dll
		${DEPDIR}/dxc/dxil.dll

		${DEPDIR}/WinPixEventRuntime/Bin/x64/WinPixEventRuntime.dll

		$<TARGET_FILE_DIR:${PROJECTNAME}>
	DEPENDS ${PROJECTNAME})

# D3D12
add_custom_command(
	TARGET ${PROJECTNAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${DEPDIR}/AgilitySDK/Bin/x64

		$<TARGET_FILE_DIR:${PROJECTNAME}>/D3D12
	DEPENDS ${PROJECTNAME})