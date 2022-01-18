cmake_minimum_required(VERSION 3.16)

set(ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Engine")
set(DEPDIR "${CMAKE_SOURCE_DIR}/Dependencies")

set(MATH_DIR "${ENGINE_DIR}/Math")
set(CORE_DIR "${ENGINE_DIR}/Core")
set(SYSTEM_DIR "${ENGINE_DIR}/System")
set(RHI_DIR "${ENGINE_DIR}/RHI")

add_subdirectory(Engine)
add_subdirectory(Application)
