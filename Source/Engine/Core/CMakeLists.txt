cmake_minimum_required(VERSION 3.16)

set(PROJECTNAME "Core")

file(GLOB_RECURSE inc ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})

add_library(${PROJECTNAME} STATIC
	${inc}
	${src})
target_precompile_headers(${PROJECTNAME} PRIVATE pch.h)

if (MSVC)
	target_compile_options(${PROJECTNAME} PRIVATE "/W3") # warning level 3
	target_compile_options(${PROJECTNAME} PRIVATE "/MP") # Multi-processor compilation
endif()

set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 23)

target_include_directories(${PROJECTNAME} PRIVATE "${ENGINE_DIR}")
target_link_libraries(${PROJECTNAME} PRIVATE "System")
target_link_libraries(${PROJECTNAME} PRIVATE "RHI")
target_link_libraries(${PROJECTNAME} PRIVATE "Math")

target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/entt/single_include/entt")
target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/json/single_include")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/assimp/include")
target_link_libraries(${PROJECTNAME} PRIVATE ${DEPDIR}/assimp/lib/Release/assimp-vc143-mt.lib)
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/DirectXMesh/include")
target_link_libraries(${PROJECTNAME} PRIVATE debug ${DEPDIR}/DirectXMesh/lib/Debug/DirectXMesh.lib)
target_link_libraries(${PROJECTNAME} PRIVATE optimized ${DEPDIR}/DirectXMesh/lib/Release/DirectXMesh.lib)
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/DirectXTex/include")
target_link_libraries(${PROJECTNAME} PRIVATE debug ${DEPDIR}/DirectXTex/lib/Debug/DirectXTex.lib)
target_link_libraries(${PROJECTNAME} PRIVATE optimized ${DEPDIR}/DirectXTex/lib/Release/DirectXTex.lib)

add_custom_command(
	TARGET ${PROJECTNAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy

		${DEPDIR}/assimp/bin/release/assimp-vc143-mt.dll

		$<TARGET_FILE_DIR:${PROJECTNAME}>
	DEPENDS ${PROJECTNAME})
