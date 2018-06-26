project(hd-maya)

cmake_minimum_required(VERSION 2.8)

option(LUMA_USD_BUILD "Build the plugin for Luma's version of USD." OFF)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory(plugin)

install(
    FILES README.md
    DESTINATION .)