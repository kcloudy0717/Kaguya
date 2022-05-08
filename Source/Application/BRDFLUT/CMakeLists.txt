cmake_minimum_required(VERSION 3.16)

set(PROJECTNAME BRDFLUT)
set(Main "main.cpp")

file(GLOB_RECURSE inc ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Main})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${inc})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})

add_executable(
	${PROJECTNAME}
	${Main}
	${inc}
	${src})
set_property(TARGET ${PROJECTNAME} PROPERTY CXX_STANDARD 23)
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
