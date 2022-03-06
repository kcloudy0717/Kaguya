cmake_minimum_required(VERSION 3.16)

set(PROJECTNAME Kaguya)
set(Main "main.cpp")

file(GLOB_RECURSE inc ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

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

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Main})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${imgui})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ImGuizmo})

add_executable(
	${PROJECTNAME}
	${Main}
	${inc}
	${src}

	${imgui}
	${ImGuizmo})
set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 23)
target_precompile_headers(${PROJECTNAME} PRIVATE pch.h)
if (MSVC)
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECTNAME}>)
	target_compile_options(${PROJECTNAME} PRIVATE "/W3") # warning level 3
	target_compile_options(${PROJECTNAME} PRIVATE "/MP") # Multi-processor compilation
endif()

add_dependencies(${PROJECTNAME} Shaders)

target_include_directories(${PROJECTNAME} PRIVATE "${ENGINE_DIR}")
target_link_libraries(${PROJECTNAME} PRIVATE "System")
target_link_libraries(${PROJECTNAME} PRIVATE "RHI")
target_link_libraries(${PROJECTNAME} PRIVATE "Math")
target_link_libraries(${PROJECTNAME} PRIVATE "Core")

target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/imgui")
target_include_directories(${PROJECTNAME} PRIVATE "${DEPDIR}/ImGuizmo")

add_custom_command(
	TARGET ${PROJECTNAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy

		${CMAKE_SOURCE_DIR}/imgui.ini

		$<TARGET_FILE_DIR:${PROJECTNAME}>
	DEPENDS ${PROJECTNAME})

add_custom_command(
	TARGET ${PROJECTNAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory

		${CMAKE_SOURCE_DIR}/Assets

		$<TARGET_FILE_DIR:${PROJECTNAME}>/Assets
	DEPENDS ${PROJECTNAME})
