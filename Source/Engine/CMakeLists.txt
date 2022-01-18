cmake_minimum_required(VERSION 3.16)

add_subdirectory(Math)
add_subdirectory(Core)
add_subdirectory(System)
add_subdirectory(RHI)

set_target_properties(Math PROPERTIES FOLDER Engine)
set_target_properties(Core PROPERTIES FOLDER Engine)
set_target_properties(System PROPERTIES FOLDER Engine)
set_target_properties(RHI PROPERTIES FOLDER Engine)

#cmake_minimum_required(VERSION 3.16)
#
#set(DEPDIR "${CMAKE_SOURCE_DIR}/Dependencies")
#
#set(PROJECTNAME Kaguya)
#set(PROJECTDIR "${CMAKE_CURRENT_SOURCE_DIR}/Engine")
#set(Main "main.cpp")
#
#file(GLOB_RECURSE inc ${CMAKE_CURRENT_SOURCE_DIR}/Engine/*.h)
#file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/Engine/*.cpp)
#
#set(imgui
#	${DEPDIR}/imgui/imconfig.h
#	${DEPDIR}/imgui/imgui.cpp
#	${DEPDIR}/imgui/imgui.h
#	${DEPDIR}/imgui/imgui_demo.cpp
#	${DEPDIR}/imgui/imgui_draw.cpp
#	${DEPDIR}/imgui/imgui_internal.h
#	${DEPDIR}/imgui/imgui_tables.cpp
#	${DEPDIR}/imgui/imgui_widgets.cpp
#	${DEPDIR}/imgui/imstb_rectpack.h
#	${DEPDIR}/imgui/imstb_textedit.h
#	${DEPDIR}/imgui/imstb_truetype.h
#	# Backend
#	${DEPDIR}/imgui/backends/imgui_impl_dx12.cpp
#	${DEPDIR}/imgui/backends/imgui_impl_dx12.h
#	${DEPDIR}/imgui/backends/imgui_impl_win32.cpp
#	${DEPDIR}/imgui/backends/imgui_impl_win32.h)
#set(ImGuizmo
#	${DEPDIR}/ImGuizmo/ImCurveEdit.cpp
#	${DEPDIR}/ImGuizmo/ImCurveEdit.h
#	${DEPDIR}/ImGuizmo/ImGradient.cpp
#	${DEPDIR}/ImGuizmo/ImGradient.h
#	${DEPDIR}/ImGuizmo/ImGuizmo.cpp
#	${DEPDIR}/ImGuizmo/ImGuizmo.h
#	${DEPDIR}/ImGuizmo/ImSequencer.cpp
#	${DEPDIR}/ImGuizmo/ImSequencer.h
#	${DEPDIR}/ImGuizmo/ImZoomSlider.h)
#set(json
#	${DEPDIR}/json/single_include/nlohmann/json.hpp)
#set(spdlog
#	${DEPDIR}/spdlog/include/spdlog/spdlog.h)
#set(dxc
#	${DEPDIR}/dxc/d3d12shader.h
#	${DEPDIR}/dxc/dxcapi.h)
#set(cityhash
#	${DEPDIR}/google/cityhash/city.h
#	${DEPDIR}/google/cityhash/city.cc)
#
#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Main})
#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})
#source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${imgui})
#source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ImGuizmo})
#source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${json})
#source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${spdlog})
#source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${dxc})
#source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${cityhash})
#
#add_executable(
#	${PROJECTNAME}
#	${Main}
#	${inc}
#	${src}
#
#	${imgui}
#	${ImGuizmo}
#	${json}
#	${spdlog}
#	${dxc}
#	${cityhash})
#
#if (MSVC)
#	target_compile_options(${PROJECTNAME} PRIVATE "/W3") # warning level 3
#	target_compile_options(${PROJECTNAME} PRIVATE "/MP") # Multi-processor compilation
#endif()
#
#file(GLOB_RECURSE inc_hlsl ${PROJECT_SOURCE_DIR}/Shaders/*.hlsli)
#file(GLOB_RECURSE src_hlsl ${PROJECT_SOURCE_DIR}/Shaders/*.hlsl)
#
#source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${inc_hlsl})
#source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${src_hlsl})
#set_source_files_properties(${src_hlsl} PROPERTIES VS_TOOL_OVERRIDE None)
#
#add_custom_target(Shaders
#	ALL SOURCES ${inc_hlsl} ${src_hlsl}
#	DEPENDS ${inc_hlsl} ${src_hlsl})
#add_custom_command(
#	TARGET Shaders
#	COMMAND ${CMAKE_COMMAND}
#	-E copy_directory ${PROJECT_SOURCE_DIR}/Shaders ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>/Shaders)
#
#add_dependencies(${PROJECTNAME} Shaders)
#
#set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 23)
#target_precompile_headers(${PROJECTNAME} PRIVATE pch.h)
#
#target_include_directories(${PROJECTNAME} PRIVATE ${PROJECTDIR})
#
#target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/entt/single_include/entt")
#target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/imgui")
#target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/ImGuizmo")
#target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/json/single_include")
#target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/spdlog/include")
#target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/wil/include")
#target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/assimp/include")
#target_link_libraries(${PROJECTNAME} PRIVATE ${DEPDIR}/assimp/lib/Release/assimp-vc143-mt.lib)
#target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/DirectXMesh/include")
#target_link_libraries(${PROJECTNAME} PRIVATE debug ${DEPDIR}/DirectXMesh/lib/Debug/DirectXMesh.lib)
#target_link_libraries(${PROJECTNAME} PRIVATE optimized ${DEPDIR}/DirectXMesh/lib/Release/DirectXMesh.lib)
#target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/DirectXTex/include")
#target_link_libraries(${PROJECTNAME} PRIVATE debug ${DEPDIR}/DirectXTex/lib/Debug/DirectXTex.lib)
#target_link_libraries(${PROJECTNAME} PRIVATE optimized ${DEPDIR}/DirectXTex/lib/Release/DirectXTex.lib)
#target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/dxc")
#target_link_libraries(${PROJECTNAME} PRIVATE ${DEPDIR}/dxc/dxcompiler.lib)
#target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/google/cityhash")
#target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/AgilitySDK/Include")
#target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/WinPixEventRuntime/Include")
#target_link_libraries(${PROJECTNAME} PRIVATE ${DEPDIR}/WinPixEventRuntime/Bin/x64/WinPixEventRuntime.lib)
## Linking
#
## DLL
#add_custom_command(
#	TARGET ${PROJECTNAME} POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy
#
#		${DEPDIR}/assimp/bin/release/assimp-vc143-mt.dll
#		${CMAKE_SOURCE_DIR}/imgui.ini
#
#		$<TARGET_FILE_DIR:${PROJECTNAME}>
#	DEPENDS ${PROJECTNAME})
#
## Copy assets
#add_custom_command(
#	TARGET ${PROJECTNAME} POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy_directory
#		${CMAKE_SOURCE_DIR}/Assets/Assets
#
#		$<TARGET_FILE_DIR:${PROJECTNAME}>/Assets
#	DEPENDS ${PROJECTNAME})
