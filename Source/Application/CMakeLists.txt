cmake_minimum_required(VERSION 3.16)

add_subdirectory(Kaguya)
add_subdirectory(BRDFLUT)

set_target_properties(Kaguya PROPERTIES FOLDER Apps)
set_target_properties(BRDFLUT PROPERTIES FOLDER Apps)
