cmake_minimum_required(VERSION 3.16)
include(FetchContent)

set(PROJECTNAME "Core")

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

target_include_directories(${PROJECTNAME} PRIVATE "${ENGINE_DIR}")
target_link_libraries(${PROJECTNAME} PRIVATE "System")
target_link_libraries(${PROJECTNAME} PRIVATE "RHI")
target_link_libraries(${PROJECTNAME} PRIVATE "Math")

target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/entt/single_include/entt")
target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/json/single_include")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/assimp/include")
target_link_libraries(${PROJECTNAME} PRIVATE ${DEPDIR}/assimp/lib/Release/assimp-vc143-mt.lib)

add_custom_command(
	TARGET ${PROJECTNAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy

		${DEPDIR}/assimp/bin/release/assimp-vc143-mt.dll

		${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
	DEPENDS ${PROJECTNAME})

# DirectXTex
message("Fetching DirectXTex")
FetchContent_Declare(directxtex URL https://www.nuget.org/api/v2/package/directxtex_desktop_win10/2022.7.30.1)
FetchContent_MakeAvailable(directxtex)
add_library(
    DirectXTex INTERFACE 
    "${directxtex_SOURCE_DIR}/include/DirectXTex.h"
    "${directxtex_SOURCE_DIR}/include/DirectXTex.inl")
target_include_directories(DirectXTex INTERFACE "${directxtex_SOURCE_DIR}/include/")
target_link_directories(DirectXTex INTERFACE ${directxtex_SOURCE_DIR}/native/lib/x64/$<CONFIG>)
target_link_libraries(DirectXTex INTERFACE "DirectXTex.lib")
target_link_libraries(${PROJECTNAME} PUBLIC DirectXTex)
set_target_properties(DirectXTex PROPERTIES FOLDER "Dependencies-Core")
message("DirectXTex fetched")

# DirectXMesh
message("Fetching DirectXMesh")
FetchContent_Declare(directxmesh URL https://www.nuget.org/api/v2/package/directxmesh_desktop_win10/2022.7.30.1)
FetchContent_MakeAvailable(directxmesh)
add_library(
	DirectXMesh INTERFACE 
	"${directxmesh_SOURCE_DIR}/include/DirectXMesh.h"
	"${directxmesh_SOURCE_DIR}/include/DirectXMesh.inl")
target_include_directories(DirectXMesh INTERFACE "${directxmesh_SOURCE_DIR}/include/")
target_link_directories(DirectXMesh INTERFACE ${directxmesh_SOURCE_DIR}/native/lib/x64/$<CONFIG>)
target_link_libraries(DirectXMesh INTERFACE "DirectXMesh.lib")
target_link_libraries(${PROJECTNAME} PUBLIC DirectXMesh)
set_target_properties(DirectXMesh PROPERTIES FOLDER "Dependencies-Core")
message("DirectXMesh fetched")
