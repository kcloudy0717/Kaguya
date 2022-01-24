cmake_minimum_required(VERSION 3.16)

add_subdirectory(Math)
add_subdirectory(Core)
add_subdirectory(System)
add_subdirectory(RHI)

set_target_properties(Math PROPERTIES FOLDER Engine)
set_target_properties(Core PROPERTIES FOLDER Engine)
set_target_properties(System PROPERTIES FOLDER Engine)
set_target_properties(RHI PROPERTIES FOLDER Engine)
