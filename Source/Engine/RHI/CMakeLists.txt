cmake_minimum_required(VERSION 3.16)
include(FetchContent)

set(PROJECTNAME "RHI")

file(GLOB_RECURSE inc ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE natvis ${CMAKE_CURRENT_SOURCE_DIR}/*.natvis)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${natvis})

add_library(${PROJECTNAME} STATIC
	${inc}
	${src}
	${natvis})

if (MSVC)
	target_compile_options(${PROJECTNAME} PRIVATE "/W3") # warning level 3
	target_compile_options(${PROJECTNAME} PRIVATE "/MP") # Multi-processor compilation
endif()

set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 23)

target_include_directories(${PROJECTNAME} PRIVATE ${RHI_DIR})
target_include_directories(${PROJECTNAME} PRIVATE ${ENGINE_DIR})
target_link_libraries(${PROJECTNAME} PRIVATE System)

# D3D12 Agility SDK
message("Fetching D3D12 Agility SDK")
FetchContent_Declare(d3d12-agility-sdk URL https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.606.3)
FetchContent_MakeAvailable(d3d12-agility-sdk)
file(GLOB_RECURSE d3d12-agility-sdk_include "${d3d12-agility-sdk_SOURCE_DIR}/build/native/include/*.h")
add_library(
	D3D12AgilitySDK INTERFACE
	${d3d12-agility-sdk_include}
	"${d3d12-agility-sdk_SOURCE_DIR}/build/native/bin/x64/D3D12Core.dll"
	"${d3d12-agility-sdk_SOURCE_DIR}/build/native/bin/x64/d3d12SDKLayers.dll")
target_include_directories(D3D12AgilitySDK INTERFACE "${d3d12-agility-sdk_SOURCE_DIR}/build/native/include")
target_link_libraries(D3D12AgilitySDK INTERFACE "d3d12.lib" "dxgi.lib")
target_link_libraries(${PROJECTNAME} PUBLIC D3D12AgilitySDK)
set_target_properties(D3D12AgilitySDK PROPERTIES FOLDER "Dependencies-RHI")

# Copy DLLs
add_custom_command(
	TARGET ${PROJECTNAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${d3d12-agility-sdk_SOURCE_DIR}/build/native/bin/x64"

	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/D3D12
	DEPENDS D3D12AgilitySDK)
message("D3D12 Agility SDK fetched")

# DXC
message("Fetching DirectXShaderCompiler")
FetchContent_Declare(dxc URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2207/dxc_2022_07_18.zip)
FetchContent_MakeAvailable(dxc)
file(MAKE_DIRECTORY ${dxc_SOURCE_DIR}/include)
file(COPY ${dxc_SOURCE_DIR}/inc/d3d12shader.h DESTINATION ${dxc_SOURCE_DIR}/include/dxc)
file(COPY ${dxc_SOURCE_DIR}/inc/dxcapi.h DESTINATION ${dxc_SOURCE_DIR}/include/dxc)
add_library(
	DirectXShaderCompiler
	INTERFACE
	"${dxc_SOURCE_DIR}/include/dxc/d3d12shader.h"
	"${dxc_SOURCE_DIR}/include/dxc/dxcapi.h"
	"${dxc_SOURCE_DIR}/bin/x64/dxcompiler.dll"
	"${dxc_SOURCE_DIR}/bin/x64/dxil.dll")
target_include_directories(DirectXShaderCompiler INTERFACE "${dxc_SOURCE_DIR}/include")
target_link_libraries(DirectXShaderCompiler INTERFACE "${dxc_SOURCE_DIR}/lib/x64/dxcompiler.lib")
target_link_libraries(${PROJECTNAME} PUBLIC DirectXShaderCompiler)
set_target_properties(DirectXShaderCompiler PROPERTIES FOLDER "Dependencies-RHI")

# Copy DLLs
add_custom_command(
	TARGET ${PROJECTNAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${dxc_SOURCE_DIR}/bin/x64/dxcompiler.dll"
	"${dxc_SOURCE_DIR}/bin/x64/dxil.dll"

	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
	DEPENDS DirectXShaderCompiler)
message("DirectXShaderCompiler fetched")

# DirectStorage
message("Fetching DirectStorage")
FetchContent_Declare(direct-storage URL https://www.nuget.org/api/v2/package/Microsoft.Direct3D.DirectStorage/1.0.2)
FetchContent_MakeAvailable(direct-storage)
file(GLOB_RECURSE direct-storage_include "${direct-storage_SOURCE_DIR}/native/include/*.h")
add_library(
	DirectStorage INTERFACE
	${direct-storage_include}
	"${direct-storage_SOURCE_DIR}/native/bin/x64/dstorage.dll"
	"${direct-storage_SOURCE_DIR}/native/bin/x64/dstoragecore.dll")
target_include_directories(DirectStorage INTERFACE "${direct-storage_SOURCE_DIR}/native/include")
target_link_libraries(DirectStorage INTERFACE "${direct-storage_SOURCE_DIR}/native/lib/x64/dstorage.lib")
target_link_libraries(${PROJECTNAME} PUBLIC DirectStorage)
set_target_properties(DirectStorage PROPERTIES FOLDER "Dependencies-RHI")

# Copy DLLs
add_custom_command(
	TARGET ${PROJECTNAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${direct-storage_SOURCE_DIR}/native/bin/x64/dstorage.dll"
	"${direct-storage_SOURCE_DIR}/native/bin/x64/dstoragecore.dll"

	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
	DEPENDS DirectStorage)
message("DirectStorage fetched")

# PIX
message("Fetching PIX")
FetchContent_Declare(pix URL https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.220124001)
FetchContent_MakeAvailable(pix)
file(GLOB_RECURSE pix_include "${pix_SOURCE_DIR}/Include/WinPixEventRuntime/*.h")
add_library(
	PIX INTERFACE
	${pix_include}
	"${pix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll")
target_include_directories(PIX INTERFACE "${pix_SOURCE_DIR}/Include/WinPixEventRuntime")
target_link_libraries(PIX INTERFACE "${pix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.lib")
target_link_libraries(${PROJECTNAME} PUBLIC PIX)
set_target_properties(PIX PROPERTIES FOLDER "Dependencies-RHI")

add_custom_command(
	TARGET ${PROJECTNAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${pix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll"

	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
	DEPENDS PIX)
message("PIX fetched")
