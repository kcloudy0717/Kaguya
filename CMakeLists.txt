cmake_minimum_required(VERSION 3.16)

# Group CMakePredefinedTargets into a folder filter
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set Windows SDK Version
set(CMAKE_SYSTEM_VERSION 10.0.19041.0)
project(Kaguya)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/Output/Lib") # Output directory for static lib (.lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/Output/Bin") # Output directory for shared lib (.dll)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/Output/Bin") # Output directory for executables (.exe)

set(CMAKE_CXX_STANDARD_REQUIRED)
add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)

add_subdirectory(Source)
