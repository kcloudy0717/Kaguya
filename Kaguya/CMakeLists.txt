cmake_minimum_required(VERSION 3.16)

find_package(Vulkan REQUIRED)

set(DEPDIR "${CMAKE_SOURCE_DIR}/Dependencies")
set(DEPONLINEDIR "${DEPDIR}/Online")
set(DEPOFFLINEDIR "${DEPDIR}/Offline")

set(PROJECTNAME Kaguya)
set(PROJECTDIR "${CMAKE_CURRENT_SOURCE_DIR}/Kaguya")
set(Main "main.cpp")

file(GLOB_RECURSE inc
${CMAKE_CURRENT_SOURCE_DIR}/Kaguya/*.h)

file(GLOB_RECURSE src
${CMAKE_CURRENT_SOURCE_DIR}/Kaguya/*.cpp)

# Online
set(imgui
	${DEPONLINEDIR}/imgui/imconfig.h
	${DEPONLINEDIR}/imgui/imgui.cpp
	${DEPONLINEDIR}/imgui/imgui.h
	${DEPONLINEDIR}/imgui/imgui_demo.cpp
	${DEPONLINEDIR}/imgui/imgui_draw.cpp
	${DEPONLINEDIR}/imgui/imgui_internal.h
	${DEPONLINEDIR}/imgui/imgui_tables.cpp
	${DEPONLINEDIR}/imgui/imgui_widgets.cpp
	${DEPONLINEDIR}/imgui/imstb_rectpack.h
	${DEPONLINEDIR}/imgui/imstb_textedit.h
	${DEPONLINEDIR}/imgui/imstb_truetype.h
	# Backend
	${DEPONLINEDIR}/imgui/backends/imgui_impl_dx12.cpp
	${DEPONLINEDIR}/imgui/backends/imgui_impl_dx12.h
	${DEPONLINEDIR}/imgui/backends/imgui_impl_win32.cpp
	${DEPONLINEDIR}/imgui/backends/imgui_impl_win32.h)
set(ImGuizmo
	${DEPONLINEDIR}/ImGuizmo/ImCurveEdit.cpp
	${DEPONLINEDIR}/ImGuizmo/ImCurveEdit.h
	${DEPONLINEDIR}/ImGuizmo/ImGradient.cpp
	${DEPONLINEDIR}/ImGuizmo/ImGradient.h
	${DEPONLINEDIR}/ImGuizmo/ImGuizmo.cpp
	${DEPONLINEDIR}/ImGuizmo/ImGuizmo.h
	${DEPONLINEDIR}/ImGuizmo/ImSequencer.cpp
	${DEPONLINEDIR}/ImGuizmo/ImSequencer.h
	${DEPONLINEDIR}/ImGuizmo/ImZoomSlider.h)
set(json
	${DEPONLINEDIR}/json/single_include/nlohmann/json.hpp)
set(spdlog
	${DEPONLINEDIR}/spdlog/include/spdlog/spdlog.h)

# Offline
set(dxc
	${DEPOFFLINEDIR}/dxc/d3d12shader.h
	${DEPOFFLINEDIR}/dxc/dxcapi.h)
set(cityhash
	${DEPOFFLINEDIR}/google/cityhash/city.h
	${DEPOFFLINEDIR}/google/cityhash/city.cc)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Main})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})
# Online
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${imgui})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ImGuizmo})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${json})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${spdlog})
# Offline
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${dxc})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${cityhash})

if (MSVC)
    add_compile_options(/W3) # warning level 3
	add_compile_options(/MP) # Multi-processor compilation
endif()

add_executable(
	${PROJECTNAME}
	${Main}
	${inc}
	${src}

	# Online
	${imgui}
	${ImGuizmo}
	${json}
	${spdlog}
	
	# Offline
	${dxc}
	${cityhash})

set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 20)
target_precompile_headers(${PROJECTNAME} PRIVATE pch.h)

include_directories(${PROJECTDIR})

# Online
include_directories("${DEPONLINEDIR}/entt/single_include/entt")
include_directories("${DEPONLINEDIR}/imgui")
include_directories("${DEPONLINEDIR}/ImGuizmo")
include_directories("${DEPONLINEDIR}/json/single_include")
include_directories("${DEPONLINEDIR}/spdlog/include")
include_directories("${DEPONLINEDIR}/wil/include")

# Offline
include_directories("${DEPOFFLINEDIR}/assimp/include")
target_link_libraries(${PROJECTNAME} ${DEPOFFLINEDIR}/assimp/lib/Release/assimp-vc142-mt.lib)

include_directories("${DEPOFFLINEDIR}/DirectXTex/include")
target_link_libraries(${PROJECTNAME} debug ${DEPOFFLINEDIR}/DirectXTex/lib/Debug/DirectXTex.lib)
target_link_libraries(${PROJECTNAME} optimized ${DEPOFFLINEDIR}/DirectXTex/lib/Release/DirectXTex.lib)

include_directories("${DEPOFFLINEDIR}/dxc")
target_link_libraries(${PROJECTNAME} ${DEPOFFLINEDIR}/dxc/dxcompiler.lib)

include_directories("${DEPOFFLINEDIR}/google/cityhash")

include_directories("${DEPOFFLINEDIR}/microsoft.direct3d.d3d12.1.4.10/build/native/include")

include_directories("${DEPOFFLINEDIR}/NVIDIA_Nsight_Aftermath_SDK_2021.1.0.21090/include")
target_link_libraries(${PROJECTNAME} ${DEPOFFLINEDIR}/NVIDIA_Nsight_Aftermath_SDK_2021.1.0.21090/lib/x64/GFSDK_Aftermath_Lib.x64.lib)

set(PhysX_SDK
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/LowLevel_static_64.lib
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/LowLevelAABB_static_64.lib
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/LowLevelDynamics_static_64.lib
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/PhysX_64.lib
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/PhysXCharacterKinematic_static_64.lib
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/PhysXCommon_64.lib
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/PhysXCooking_64.lib
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/PhysXExtensions_static_64.lib
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/PhysXFoundation_64.lib
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/PhysXPvdSDK_static_64.lib
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/PhysXTask_static_64.lib
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/PhysXVehicle_static_64.lib
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/SceneQuery_static_64.lib
	debug ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/debug/SimulationController_static_64.lib

	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/LowLevel_static_64.lib
	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/LowLevelAABB_static_64.lib
	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/LowLevelDynamics_static_64.lib
	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/PhysX_64.lib
	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/PhysXCharacterKinematic_static_64.lib
	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/PhysXCommon_64.lib
	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/PhysXCooking_64.lib
	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/PhysXExtensions_static_64.lib
	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/PhysXFoundation_64.lib
	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/PhysXPvdSDK_static_64.lib
	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/PhysXTask_static_64.lib
	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/PhysXVehicle_static_64.lib
	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/SceneQuery_static_64.lib
	optimized ${DEPOFFLINEDIR}/PhysX-4.0.1/bin/release/SimulationController_static_64.lib)
include_directories("${DEPOFFLINEDIR}/PhysX-4.0.1/include")
target_link_libraries(${PROJECTNAME} ${PhysX_SDK})

include_directories("${DEPOFFLINEDIR}/winpixeventruntime.1.0.210209001/Include/WinPixEventRuntime")
target_link_libraries(${PROJECTNAME} ${DEPOFFLINEDIR}/winpixeventruntime.1.0.210209001/bin/x64/WinPixEventRuntime.lib)

# Linking
include_directories(Vulkan::Headers)
target_link_libraries(${PROJECTNAME} Vulkan::Vulkan)

# Offline DLL
add_custom_command(
TARGET ${PROJECTNAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy 
	${DEPOFFLINEDIR}/assimp/bin/release/assimp-vc142-mt.dll

	${DEPOFFLINEDIR}/dxc/dxcompiler.dll
	${DEPOFFLINEDIR}/dxc/dxil.dll

	${DEPOFFLINEDIR}/NVIDIA_Nsight_Aftermath_SDK_2021.1.0.21090/lib/x64/GFSDK_Aftermath_Lib.x64.dll
	${DEPOFFLINEDIR}/NVIDIA_Nsight_Aftermath_SDK_2021.1.0.21090/lib/x64/llvm_7_0_1.dll

	${DEPOFFLINEDIR}/winpixeventruntime.1.0.210209001/bin/x64/WinPixEventRuntime.dll

	${CMAKE_SOURCE_DIR}/imgui.ini

	$<TARGET_FILE_DIR:${PROJECTNAME}>
DEPENDS ${PROJECTNAME})

# D3D12
add_custom_command(
TARGET ${PROJECTNAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${DEPOFFLINEDIR}/microsoft.direct3d.d3d12.1.4.10/build/native/bin/x64

	$<TARGET_FILE_DIR:${PROJECTNAME}>/D3D12
DEPENDS ${PROJECTNAME})

# PhysX
add_custom_command(
TARGET ${PROJECTNAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy 
	${DEPOFFLINEDIR}/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysX_64.dll
	${DEPOFFLINEDIR}/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXCommon_64.dll
	${DEPOFFLINEDIR}/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXCooking_64.dll
	${DEPOFFLINEDIR}/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXDevice64.dll
	${DEPOFFLINEDIR}/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXFoundation_64.dll
	${DEPOFFLINEDIR}/PhysX-4.0.1/bin/$<CONFIGURATION>/PhysXGpu_64.dll

	$<TARGET_FILE_DIR:${PROJECTNAME}>
DEPENDS ${PROJECTNAME})

# Copy assets
add_custom_command(
TARGET ${PROJECTNAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/Assets

	$<TARGET_FILE_DIR:${PROJECTNAME}>/Assets
DEPENDS ${PROJECTNAME})

# Copy shaders
add_custom_command(
TARGET ${PROJECTNAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/Shaders

	$<TARGET_FILE_DIR:${PROJECTNAME}>/Shaders
DEPENDS ${PROJECTNAME})
