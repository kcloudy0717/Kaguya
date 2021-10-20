cmake_minimum_required(VERSION 3.16)

set(DEPDIR "${CMAKE_SOURCE_DIR}/Dependencies")

set(PROJECTNAME Kaguya)
set(PROJECTDIR "${CMAKE_CURRENT_SOURCE_DIR}/Engine")
set(Main "main.cpp")

file(GLOB_RECURSE inc
${CMAKE_CURRENT_SOURCE_DIR}/Engine/*.h)

file(GLOB_RECURSE src
${CMAKE_CURRENT_SOURCE_DIR}/Engine/*.cpp)

set(imgui
	${DEPDIR}/imgui/imconfig.h
	${DEPDIR}/imgui/imgui.cpp
	${DEPDIR}/imgui/imgui.h
	${DEPDIR}/imgui/imgui_demo.cpp
	${DEPDIR}/imgui/imgui_draw.cpp
	${DEPDIR}/imgui/imgui_internal.h
	${DEPDIR}/imgui/imgui_tables.cpp
	${DEPDIR}/imgui/imgui_widgets.cpp
	${DEPDIR}/imgui/imstb_rectpack.h
	${DEPDIR}/imgui/imstb_textedit.h
	${DEPDIR}/imgui/imstb_truetype.h
	# Backend
	${DEPDIR}/imgui/backends/imgui_impl_dx12.cpp
	${DEPDIR}/imgui/backends/imgui_impl_dx12.h
	${DEPDIR}/imgui/backends/imgui_impl_win32.cpp
	${DEPDIR}/imgui/backends/imgui_impl_win32.h)
set(ImGuizmo
	${DEPDIR}/ImGuizmo/ImCurveEdit.cpp
	${DEPDIR}/ImGuizmo/ImCurveEdit.h
	${DEPDIR}/ImGuizmo/ImGradient.cpp
	${DEPDIR}/ImGuizmo/ImGradient.h
	${DEPDIR}/ImGuizmo/ImGuizmo.cpp
	${DEPDIR}/ImGuizmo/ImGuizmo.h
	${DEPDIR}/ImGuizmo/ImSequencer.cpp
	${DEPDIR}/ImGuizmo/ImSequencer.h
	${DEPDIR}/ImGuizmo/ImZoomSlider.h)
set(json
	${DEPDIR}/json/single_include/nlohmann/json.hpp)
set(spdlog
	${DEPDIR}/spdlog/include/spdlog/spdlog.h)
set(dxc
	${DEPDIR}/dxc/d3d12shader.h
	${DEPDIR}/dxc/dxcapi.h)
set(cityhash
	${DEPDIR}/google/cityhash/city.h
	${DEPDIR}/google/cityhash/city.cc)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Main})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${imgui})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ImGuizmo})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${json})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${spdlog})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${dxc})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${cityhash})

add_compile_options(/W3) # warning level 3
add_compile_options(/MP) # Multi-processor compilation

add_executable(
	${PROJECTNAME}
	${Main}
	${inc}
	${src}

	${imgui}
	${ImGuizmo}
	${json}
	${spdlog}
	${dxc}
	${cityhash})

file(GLOB_RECURSE inc_hlsl
${PROJECT_SOURCE_DIR}/Shaders/*.hlsli)

file(GLOB_RECURSE src_hlsl
${PROJECT_SOURCE_DIR}/Shaders/*.hlsl)

source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${inc_hlsl})
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${src_hlsl})
set_source_files_properties(${src_hlsl} PROPERTIES VS_TOOL_OVERRIDE None)

add_custom_target(Shaders
ALL SOURCES ${inc_hlsl} ${src_hlsl}
DEPENDS ${inc_hlsl} ${src_hlsl})
add_custom_command(
TARGET Shaders
COMMAND ${CMAKE_COMMAND}
-E copy_directory ${PROJECT_SOURCE_DIR}/Shaders ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>/Shaders)

add_dependencies(${PROJECTNAME} Shaders)

set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 23)
target_precompile_headers(${PROJECTNAME} PRIVATE pch.h)

target_include_directories(${PROJECTNAME} PUBLIC ${PROJECTDIR})

target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/entt/single_include/entt")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/imgui")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/ImGuizmo")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/json/single_include")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/spdlog/include")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/wil/include")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/assimp/include")
target_link_libraries(${PROJECTNAME} ${DEPDIR}/assimp/lib/Release/assimp-vc142-mt.lib)
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/DirectXMesh/include")
target_link_libraries(${PROJECTNAME} debug ${DEPDIR}/DirectXMesh/lib/Debug/DirectXMesh.lib)
target_link_libraries(${PROJECTNAME} optimized ${DEPDIR}/DirectXMesh/lib/Release/DirectXMesh.lib)
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/DirectXTex/include")
target_link_libraries(${PROJECTNAME} debug ${DEPDIR}/DirectXTex/lib/Debug/DirectXTex.lib)
target_link_libraries(${PROJECTNAME} optimized ${DEPDIR}/DirectXTex/lib/Release/DirectXTex.lib)
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/dxc")
target_link_libraries(${PROJECTNAME} ${DEPDIR}/dxc/dxcompiler.lib)
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/google/cityhash")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/microsoft.direct3d.d3d12.1.4.10/build/native/include")
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/NVIDIA_Nsight_Aftermath_SDK_2021.1.0.21090/include")
target_link_libraries(${PROJECTNAME} ${DEPDIR}/NVIDIA_Nsight_Aftermath_SDK_2021.1.0.21090/lib/x64/GFSDK_Aftermath_Lib.x64.lib)

set(PhysX_SDK
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/LowLevel_static_64.lib
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/LowLevelAABB_static_64.lib
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/LowLevelDynamics_static_64.lib
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/PhysX_64.lib
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/PhysXCharacterKinematic_static_64.lib
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/PhysXCommon_64.lib
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/PhysXCooking_64.lib
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/PhysXExtensions_static_64.lib
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/PhysXFoundation_64.lib
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/PhysXPvdSDK_static_64.lib
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/PhysXTask_static_64.lib
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/PhysXVehicle_static_64.lib
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/SceneQuery_static_64.lib
	debug ${DEPDIR}/PhysX-4.0.1/bin/debug/SimulationController_static_64.lib

	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/LowLevel_static_64.lib
	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/LowLevelAABB_static_64.lib
	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/LowLevelDynamics_static_64.lib
	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/PhysX_64.lib
	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/PhysXCharacterKinematic_static_64.lib
	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/PhysXCommon_64.lib
	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/PhysXCooking_64.lib
	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/PhysXExtensions_static_64.lib
	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/PhysXFoundation_64.lib
	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/PhysXPvdSDK_static_64.lib
	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/PhysXTask_static_64.lib
	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/PhysXVehicle_static_64.lib
	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/SceneQuery_static_64.lib
	optimized ${DEPDIR}/PhysX-4.0.1/bin/release/SimulationController_static_64.lib)
target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/PhysX-4.0.1/include")
target_link_libraries(${PROJECTNAME} ${PhysX_SDK})

target_include_directories(${PROJECTNAME} PUBLIC "${DEPDIR}/winpixeventruntime.1.0.210818001/Include/WinPixEventRuntime")
target_link_libraries(${PROJECTNAME} ${DEPDIR}/winpixeventruntime.1.0.210818001/bin/x64/WinPixEventRuntime.lib)

# Linking

# DLL
add_custom_command(
TARGET ${PROJECTNAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy 
	${DEPDIR}/assimp/bin/release/assimp-vc142-mt.dll

	${DEPDIR}/dxc/dxcompiler.dll
	${DEPDIR}/dxc/dxil.dll

	${DEPDIR}/NVIDIA_Nsight_Aftermath_SDK_2021.1.0.21090/lib/x64/GFSDK_Aftermath_Lib.x64.dll
	${DEPDIR}/NVIDIA_Nsight_Aftermath_SDK_2021.1.0.21090/lib/x64/llvm_7_0_1.dll

	${DEPDIR}/winpixeventruntime.1.0.210818001/bin/x64/WinPixEventRuntime.dll

	${CMAKE_SOURCE_DIR}/imgui.ini

	$<TARGET_FILE_DIR:${PROJECTNAME}>
DEPENDS ${PROJECTNAME})

# D3D12
add_custom_command(
TARGET ${PROJECTNAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${DEPDIR}/microsoft.direct3d.d3d12.1.4.10/build/native/bin/x64

	$<TARGET_FILE_DIR:${PROJECTNAME}>/D3D12
DEPENDS ${PROJECTNAME})

# PhysX
add_custom_command(
TARGET ${PROJECTNAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy 
	${DEPDIR}/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysX_64.dll
	${DEPDIR}/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXCommon_64.dll
	${DEPDIR}/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXCooking_64.dll
	${DEPDIR}/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXDevice64.dll
	${DEPDIR}/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXFoundation_64.dll
	${DEPDIR}/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXGpu_64.dll

	$<TARGET_FILE_DIR:${PROJECTNAME}>
DEPENDS ${PROJECTNAME})

# Copy assets
add_custom_command(
TARGET ${PROJECTNAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/Assets/Assets

	$<TARGET_FILE_DIR:${PROJECTNAME}>/Assets
DEPENDS ${PROJECTNAME})
