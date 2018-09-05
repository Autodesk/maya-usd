project(hd-maya)

cmake_minimum_required(VERSION 2.8)

if ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" VERSION_GREATER "3.11")
    cmake_policy(SET CMP0074 OLD)
endif ()

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif ()

option(LUMA_USD_BUILD "Build the plugin for Luma's version of USD." OFF)
option(USD_HDST_SHADOWS_BUILD "Enable PR-541 for USD." OFF)
option(USD_HDST_UDIM_BUILD "Enable PR-597 for USD." OFF)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
add_compile_options(-Wall -Werror)
if (LINUX)
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined")
endif ()

add_subdirectory(src)

install(
    FILES README.md
    DESTINATION .)

install(
    FILES LICENSE.txt
    DESTINATION .)