cmake_minimum_required(VERSION 3.16)

# Group CMakePredefinedTargets into a folder filter
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set Windows SDK Version
set(CMAKE_SYSTEM_VERSION 10.0.19041.0)
project(Kaguya LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/Bin")
set(CMAKE_CXX_STANDARD_REQUIRED)
add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)

add_subdirectory(Source)
