project(hd-maya)

cmake_minimum_required(VERSION 2.8)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(plugin)

install(
    FILES README.md
    DESTINATION .)