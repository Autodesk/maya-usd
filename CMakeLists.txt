project(hd-maya)

cmake_minimum_required(VERSION 2.8)

if ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" VERSION_GREATER "3.11")
    cmake_policy(SET CMP0074 OLD)
endif ()

option(LUMA_USD_BUILD "Build the plugin for Luma's version of USD." OFF)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
add_compile_options(-Wall -Werror)

add_subdirectory(src)

install(
    FILES README.md
    DESTINATION .)

install(
    FILES LICENSE.txt
    DESTINATION .)