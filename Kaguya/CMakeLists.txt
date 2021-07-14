cmake_minimum_required(VERSION 3.16)

set(PROJECTNAME Kaguya)
set(PROJECTDIR "${CMAKE_CURRENT_SOURCE_DIR}/Kaguya")
set(Main "main.cpp")

file(GLOB_RECURSE inc
${CMAKE_CURRENT_SOURCE_DIR}/Kaguya/*.h)

file(GLOB_RECURSE src
${CMAKE_CURRENT_SOURCE_DIR}/Kaguya/*.cpp)

# ext
set(cityhash
	${CMAKE_SOURCE_DIR}/ext/google/cityhash/city.h
	${CMAKE_SOURCE_DIR}/ext/google/cityhash/city.cc)
set(DXC
	${CMAKE_SOURCE_DIR}/ext/dxc/d3d12shader.h
	${CMAKE_SOURCE_DIR}/ext/dxc/dxcapi.h)
set(IMGUI
	${CMAKE_SOURCE_DIR}/ext/imgui/imconfig.h
	${CMAKE_SOURCE_DIR}/ext/imgui/imgui.cpp
	${CMAKE_SOURCE_DIR}/ext/imgui/imgui.h
	${CMAKE_SOURCE_DIR}/ext/imgui/imgui_demo.cpp
	${CMAKE_SOURCE_DIR}/ext/imgui/imgui_draw.cpp
	${CMAKE_SOURCE_DIR}/ext/imgui/imgui_internal.h
	${CMAKE_SOURCE_DIR}/ext/imgui/imgui_tables.cpp
	${CMAKE_SOURCE_DIR}/ext/imgui/imgui_widgets.cpp
	${CMAKE_SOURCE_DIR}/ext/imgui/imstb_rectpack.h
	${CMAKE_SOURCE_DIR}/ext/imgui/imstb_textedit.h
	${CMAKE_SOURCE_DIR}/ext/imgui/imstb_truetype.h
	# Backend
	${CMAKE_SOURCE_DIR}/ext/imgui/backends/imgui_impl_dx12.cpp
	${CMAKE_SOURCE_DIR}/ext/imgui/backends/imgui_impl_dx12.h
	${CMAKE_SOURCE_DIR}/ext/imgui/backends/imgui_impl_win32.cpp
	${CMAKE_SOURCE_DIR}/ext/imgui/backends/imgui_impl_win32.h)
set(IMGUIZMO
	${CMAKE_SOURCE_DIR}/ext/ImGuizmo/ImCurveEdit.cpp
	${CMAKE_SOURCE_DIR}/ext/ImGuizmo/ImCurveEdit.h
	${CMAKE_SOURCE_DIR}/ext/ImGuizmo/ImGradient.cpp
	${CMAKE_SOURCE_DIR}/ext/ImGuizmo/ImGradient.h
	${CMAKE_SOURCE_DIR}/ext/ImGuizmo/ImGuizmo.cpp
	${CMAKE_SOURCE_DIR}/ext/ImGuizmo/ImGuizmo.h
	${CMAKE_SOURCE_DIR}/ext/ImGuizmo/ImSequencer.cpp
	${CMAKE_SOURCE_DIR}/ext/ImGuizmo/ImSequencer.h
	${CMAKE_SOURCE_DIR}/ext/ImGuizmo/ImZoomSlider.h)
set(SPDLOG
	${CMAKE_SOURCE_DIR}/ext/spdlog/include/spdlog/spdlog.h)
file(GLOB_RECURSE yaml_cpp
	${CMAKE_SOURCE_DIR}/ext/yaml-cpp/include/yaml-cpp/*.h
	${CMAKE_SOURCE_DIR}/ext/yaml-cpp/src/*.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Main})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${cityhash})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${DXC})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${IMGUI})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${IMGUIZMO})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SPDLOG})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${yaml_cpp})

if (MSVC)
    add_compile_options(/W3) # warning level 3
	add_compile_options(/MP) # Multi-processor compilation
endif()

add_executable(
	${PROJECTNAME}
	${Main}
	${inc}
	${src}

	${cityhash}
	${DXC}
	${IMGUI}
	${IMGUIZMO}
	${SPDLOG}
	${yaml_cpp})

set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 20)
target_precompile_headers(${PROJECTNAME} PRIVATE pch.h)

include_directories(${PROJECTDIR})

# ext
set(PhysX_SDK
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/LowLevel_static_64.lib
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/LowLevelAABB_static_64.lib
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/LowLevelDynamics_static_64.lib
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/PhysX_64.lib
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/PhysXCharacterKinematic_static_64.lib
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/PhysXCommon_64.lib
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/PhysXCooking_64.lib
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/PhysXExtensions_static_64.lib
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/PhysXFoundation_64.lib
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/PhysXPvdSDK_static_64.lib
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/PhysXTask_static_64.lib
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/PhysXVehicle_static_64.lib
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/SceneQuery_static_64.lib
	debug ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/debug/SimulationController_static_64.lib

	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/LowLevel_static_64.lib
	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/LowLevelAABB_static_64.lib
	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/LowLevelDynamics_static_64.lib
	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/PhysX_64.lib
	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/PhysXCharacterKinematic_static_64.lib
	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/PhysXCommon_64.lib
	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/PhysXCooking_64.lib
	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/PhysXExtensions_static_64.lib
	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/PhysXFoundation_64.lib
	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/PhysXPvdSDK_static_64.lib
	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/PhysXTask_static_64.lib
	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/PhysXVehicle_static_64.lib
	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/SceneQuery_static_64.lib
	optimized ${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/release/SimulationController_static_64.lib)

include_directories("${CMAKE_SOURCE_DIR}/ext/assimp/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/DirectXTex/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/google/cityhash")
include_directories("${CMAKE_SOURCE_DIR}/ext/dxc")
include_directories("${CMAKE_SOURCE_DIR}/ext/entt/single_include/entt")
include_directories("${CMAKE_SOURCE_DIR}/ext/imgui")
include_directories("${CMAKE_SOURCE_DIR}/ext/ImGuizmo")
include_directories("${CMAKE_SOURCE_DIR}/ext/microsoft.direct3d.d3d12.1.4.10/build/native/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/spdlog/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/wil/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/winpixeventruntime.1.0.210209001/Include/WinPixEventRuntime")
include_directories("${CMAKE_SOURCE_DIR}/ext/yaml-cpp/include")

target_link_libraries(${PROJECTNAME} ${CMAKE_SOURCE_DIR}/ext/assimp/lib/Release/assimp-vc142-mt.lib)
target_link_libraries(${PROJECTNAME} debug ${CMAKE_SOURCE_DIR}/ext/DirectXTex/lib/Debug/DirectXTex.lib)
target_link_libraries(${PROJECTNAME} optimized ${CMAKE_SOURCE_DIR}/ext/DirectXTex/lib/Release/DirectXTex.lib)
target_link_libraries(${PROJECTNAME} ${CMAKE_SOURCE_DIR}/ext/dxc/dxcompiler.lib)
target_link_libraries(${PROJECTNAME} ${PhysX_SDK})
target_link_libraries(${PROJECTNAME} ${CMAKE_SOURCE_DIR}/ext/winpixeventruntime.1.0.210209001/bin/x64/WinPixEventRuntime.lib)

# ext
# copy dlls
add_custom_command(
TARGET ${PROJECTNAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy 
	${CMAKE_SOURCE_DIR}/ext/assimp/bin/release/assimp-vc142-mt.dll
	${CMAKE_SOURCE_DIR}/ext/dxc/dxcompiler.dll
	${CMAKE_SOURCE_DIR}/ext/dxc/dxil.dll
	${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysX_64.dll
	${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXCommon_64.dll
	${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXCooking_64.dll
	${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXDevice64.dll
	${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXFoundation_64.dll
	${CMAKE_SOURCE_DIR}/ext/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXGpu_64.dll
	${CMAKE_SOURCE_DIR}/ext/winpixeventruntime.1.0.210209001/bin/x64/WinPixEventRuntime.dll
	${CMAKE_SOURCE_DIR}/imgui.ini

	$<TARGET_FILE_DIR:${PROJECTNAME}>
DEPENDS ${PROJECTNAME})

add_custom_command(
TARGET ${PROJECTNAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/ext/microsoft.direct3d.d3d12.1.4.10/build/native/bin/x64

	$<TARGET_FILE_DIR:${PROJECTNAME}>/D3D12
DEPENDS ${PROJECTNAME})

# copy assets
add_custom_command(
TARGET ${PROJECTNAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/assets

	$<TARGET_FILE_DIR:${PROJECTNAME}>/Assets
DEPENDS ${PROJECTNAME})

# copy shaders
add_custom_command(
TARGET ${PROJECTNAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/shaders

	$<TARGET_FILE_DIR:${PROJECTNAME}>/Shaders
DEPENDS ${PROJECTNAME})
