project(hd-maya-al)

cmake_minimum_required(VERSION 2.8)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Maya REQUIRED)
find_package(MayaToHydra REQUIRED)

add_subdirectory(src)
add_subdirectory(python)