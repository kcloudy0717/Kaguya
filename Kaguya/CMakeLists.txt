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

set(D3D12MemoryAllocator
${CMAKE_SOURCE_DIR}/ext/D3D12MemoryAllocator/src/D3D12MemAlloc.h
${CMAKE_SOURCE_DIR}/ext/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp)

set(entt ${CMAKE_SOURCE_DIR}/ext/entt/single_include/entt/entt.hpp)

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

# ext
include_directories("${CMAKE_SOURCE_DIR}/ext/assimp/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/D3D12MemoryAllocator/src")
include_directories("${CMAKE_SOURCE_DIR}/ext/DirectXTex/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/DirectXTK12/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/dxc")
include_directories("${CMAKE_SOURCE_DIR}/ext/entt/single_include/entt")
include_directories("${CMAKE_SOURCE_DIR}/ext/imgui")
include_directories("${CMAKE_SOURCE_DIR}/ext/ImGuizmo")
include_directories("${CMAKE_SOURCE_DIR}/ext/microsoft.direct3d.d3d12.1.4.9/build/native/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/nativefiledialog/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/spdlog/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/wil/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/WinPixEventRuntime/include")
include_directories("${CMAKE_SOURCE_DIR}/ext/yaml-cpp/include")

target_link_libraries(${PROJECTNAME} ${CMAKE_SOURCE_DIR}/ext/assimp/lib/Release/assimp-vc142-mt.lib)
target_link_libraries(${PROJECTNAME} debug ${CMAKE_SOURCE_DIR}/ext/DirectXTex/lib/Debug/DirectXTex.lib)
target_link_libraries(${PROJECTNAME} optimized ${CMAKE_SOURCE_DIR}/ext/DirectXTex/lib/Release/DirectXTex.lib)
target_link_libraries(${PROJECTNAME} debug ${CMAKE_SOURCE_DIR}/ext/DirectXTK12/lib/Debug/DirectXTK12.lib)
target_link_libraries(${PROJECTNAME} optimized ${CMAKE_SOURCE_DIR}/ext/DirectXTK12/lib/Release/DirectXTK12.lib)
target_link_libraries(${PROJECTNAME} ${CMAKE_SOURCE_DIR}/ext/dxc/dxcompiler.lib)
target_link_libraries(${PROJECTNAME} debug ${CMAKE_SOURCE_DIR}/ext/nativefiledialog/lib/Debug/nfd.lib)
target_link_libraries(${PROJECTNAME} optimized ${CMAKE_SOURCE_DIR}/ext/nativefiledialog/lib/Release/nfd.lib)
target_link_libraries(${PROJECTNAME} ${CMAKE_SOURCE_DIR}/ext/WinPixEventRuntime/bin/x64/WinPixEventRuntime.lib)

# copy dlls
add_custom_command(
TARGET ${PROJECTNAME} PRE_BUILD
COMMAND ${CMAKE_COMMAND} -E copy 
	${CMAKE_SOURCE_DIR}/ext/assimp/bin/release/assimp-vc142-mt.dll
	${CMAKE_SOURCE_DIR}/ext/dxc/dxcompiler.dll
	${CMAKE_SOURCE_DIR}/ext/dxc/dxil.dll
	${CMAKE_SOURCE_DIR}/ext/WinPixEventRuntime/bin/x64/WinPixEventRuntime.dll
	${CMAKE_SOURCE_DIR}/imgui.ini
	$<TARGET_FILE_DIR:${PROJECTNAME}>
DEPENDS ${PROJECTNAME})

add_custom_command(
TARGET ${PROJECTNAME} PRE_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/ext/microsoft.direct3d.d3d12.1.4.9/build/native/bin/x64
	$<TARGET_FILE_DIR:${PROJECTNAME}>/D3D12
DEPENDS ${PROJECTNAME})

# copy assets
add_custom_command(
TARGET ${PROJECTNAME} PRE_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/assets
	$<TARGET_FILE_DIR:${PROJECTNAME}>/assets
DEPENDS ${PROJECTNAME})

# copy shaders
add_custom_command(
TARGET ${PROJECTNAME} PRE_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/shaders
	$<TARGET_FILE_DIR:${PROJECTNAME}>/shaders
DEPENDS ${PROJECTNAME})