cmake_minimum_required(VERSION 3.15)

set(PROJECTNAME Kaguya)
set(Main "main.cpp")
set(PCH_Header "pch.h")
set(PCH_Source "pch.h" "pch.cpp")
set_source_files_properties("pch.cpp" PROPERTIES COMPILE_FLAGS "/Ycpch.h")

file(GLOB_RECURSE HeaderList
Core/*.h
Graphics/*.h
Core/*.inl
Graphics/*.inl)

file(GLOB_RECURSE SourceList
Core/*.cpp
Graphics/*.cpp)

set(D3D12MemoryAllocator_PATH ${CMAKE_SOURCE_DIR}/ext/D3D12MemoryAllocator/src)
set(D3D12MemoryAllocator
${D3D12MemoryAllocator_PATH}/D3D12MemAlloc.h
${D3D12MemoryAllocator_PATH}/D3D12MemAlloc.cpp)

set(entt_PATH ${CMAKE_SOURCE_DIR}/ext/entt/single_include/entt)
set(entt ${entt_PATH}/entt.hpp)

set(IMGUI_FILE_PATH ${CMAKE_SOURCE_DIR}/ext/imgui)
set(IMGUI
${IMGUI_FILE_PATH}/imconfig.h
${IMGUI_FILE_PATH}/imgui.cpp
${IMGUI_FILE_PATH}/imgui.h
${IMGUI_FILE_PATH}/imgui_demo.cpp
${IMGUI_FILE_PATH}/imgui_draw.cpp
${IMGUI_FILE_PATH}/imgui_internal.h
${IMGUI_FILE_PATH}/imgui_tables.cpp
${IMGUI_FILE_PATH}/imgui_widgets.cpp
${IMGUI_FILE_PATH}/imstb_rectpack.h
${IMGUI_FILE_PATH}/imstb_textedit.h
${IMGUI_FILE_PATH}/imstb_truetype.h
# Backend
${IMGUI_FILE_PATH}/backends/imgui_impl_dx12.cpp
${IMGUI_FILE_PATH}/backends/imgui_impl_dx12.h
${IMGUI_FILE_PATH}/backends/imgui_impl_win32.cpp
${IMGUI_FILE_PATH}/backends/imgui_impl_win32.h)

set(IMGUIZMO_FILE_PATH ${CMAKE_SOURCE_DIR}/ext/ImGuizmo)
set(IMGUIZMO
${IMGUIZMO_FILE_PATH}/ImCurveEdit.cpp
${IMGUIZMO_FILE_PATH}/ImCurveEdit.h
${IMGUIZMO_FILE_PATH}/ImGradient.cpp
${IMGUIZMO_FILE_PATH}/ImGradient.h
${IMGUIZMO_FILE_PATH}/ImGuizmo.cpp
${IMGUIZMO_FILE_PATH}/ImGuizmo.h
${IMGUIZMO_FILE_PATH}/ImSequencer.cpp
${IMGUIZMO_FILE_PATH}/ImSequencer.h
${IMGUIZMO_FILE_PATH}/ImZoomSlider.h)

set(SPDLOG_FILE_PATH ${CMAKE_SOURCE_DIR}/ext/spdlog)
set(SPDLOG
${SPDLOG_FILE_PATH}/include/spdlog/spdlog.h)

file(GLOB_RECURSE yaml_cpp
${CMAKE_SOURCE_DIR}/ext/yaml-cpp/include/yaml-cpp/*.h
${CMAKE_SOURCE_DIR}/ext/yaml-cpp/src/*.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Main})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PCH_Header})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PCH_Source})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HeaderList})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SourceList})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${D3D12MemoryAllocator})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${entt})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${IMGUI})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${IMGUIZMO})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SPDLOG})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${yaml_cpp})

foreach(Source IN LISTS SourceList)
    set_source_files_properties("${Source}" PROPERTIES COMPILE_FLAGS "/Yupch.h")
endforeach()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")
include_directories(".")
add_executable(
${PROJECTNAME}
${Main}
${PCH_Header}
${PCH_Source}
${HeaderList}
${SourceList}
${D3D12MemoryAllocator}
${entt}
${IMGUI}
${IMGUIZMO}
${SPDLOG}
${yaml_cpp})

set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 20)

# Submodules
include_directories("${CMAKE_SOURCE_DIR}/ext/D3D12MemoryAllocator/src")
include_directories("${CMAKE_SOURCE_DIR}/ext/entt/single_include/entt")
include_directories("${CMAKE_SOURCE_DIR}/ext/imgui")
include_directories("${CMAKE_SOURCE_DIR}/ext/ImGuizmo")
include_directories("${CMAKE_SOURCE_DIR}/ext/spdlog/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/wil/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/yaml-cpp/include")

# ext
include_directories("${CMAKE_SOURCE_DIR}/ext/assimp/include")
target_link_libraries(${PROJECTNAME} ${CMAKE_SOURCE_DIR}/ext/assimp/lib/Release/assimp-vc142-mt.lib)

include_directories("${CMAKE_SOURCE_DIR}/ext/DirectXTex/include")
target_link_libraries(${PROJECTNAME} debug ${CMAKE_SOURCE_DIR}/ext/DirectXTex/lib/Debug/DirectXTex.lib)
target_link_libraries(${PROJECTNAME} optimized ${CMAKE_SOURCE_DIR}/ext/DirectXTex/lib/Release/DirectXTex.lib)

include_directories("${CMAKE_SOURCE_DIR}/ext/DirectXTK12/include")
target_link_libraries(${PROJECTNAME} debug ${CMAKE_SOURCE_DIR}/ext/DirectXTK12/lib/Debug/DirectXTK12.lib)
target_link_libraries(${PROJECTNAME} optimized ${CMAKE_SOURCE_DIR}/ext/DirectXTK12/lib/Release/DirectXTK12.lib)

include_directories("${CMAKE_SOURCE_DIR}/ext/dxc")
target_link_libraries(${PROJECTNAME} ${CMAKE_SOURCE_DIR}/ext/dxc/dxcompiler.lib)

include_directories("${CMAKE_SOURCE_DIR}/ext/nativefiledialog/include")
target_link_libraries(${PROJECTNAME} debug ${CMAKE_SOURCE_DIR}/ext/nativefiledialog/lib/Debug/nfd.lib)
target_link_libraries(${PROJECTNAME} optimized ${CMAKE_SOURCE_DIR}/ext/nativefiledialog/lib/Release/nfd.lib)

include_directories("${CMAKE_SOURCE_DIR}/ext/WinPixEventRuntime/include")
target_link_libraries(${PROJECTNAME} ${CMAKE_SOURCE_DIR}/ext/WinPixEventRuntime/bin/x64/WinPixEventRuntime.lib)

add_custom_command(
TARGET ${PROJECTNAME} PRE_BUILD
COMMAND ${CMAKE_COMMAND} -E copy 
	${CMAKE_SOURCE_DIR}/ext/assimp/bin/release/assimp-vc142-mt.dll
	${CMAKE_SOURCE_DIR}/ext/dxc/dxcompiler.dll
	${CMAKE_SOURCE_DIR}/ext/dxc/dxil.dll
	${CMAKE_SOURCE_DIR}/ext/WinPixEventRuntime/bin/x64/WinPixEventRuntime.dll
	$<TARGET_FILE_DIR:${PROJECTNAME}>
DEPENDS ${PROJECTNAME})

add_custom_command(
TARGET ${PROJECTNAME} PRE_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/assets
	$<TARGET_FILE_DIR:${PROJECTNAME}>/assets
DEPENDS ${PROJECTNAME})

add_custom_command(
TARGET ${PROJECTNAME} PRE_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/shaders
	$<TARGET_FILE_DIR:${PROJECTNAME}>/shaders
DEPENDS ${PROJECTNAME})