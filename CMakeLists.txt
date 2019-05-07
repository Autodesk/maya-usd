project(hd-maya)

cmake_minimum_required(VERSION 2.8)

if ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" VERSION_GREATER "3.11")
    cmake_policy(SET CMP0074 OLD)
endif ()

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif ()

set(MTOH_CXX_STD "11"
    CACHE
    STRING
    "Which C++ standard / version to use, as an integer - ie, 14 means use c++14."
)

option(LUMA_USD_BUILD "Build the plugin for Luma's version of USD." OFF)
option(INSTALL_MAYA_MOD "Installs a maya module file for the plugin." OFF)
option(HDMAYA_BUILD_TESTS "Build the maya-to-hydra tests" OFF)
option(HDMAYA_UFE_BUILD "Whether to build with UFE support." OFF)
option(BUILD_DOCS "Wether to build the Documentation." OFF)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_options(/Wall)
else ()
    add_compile_options(-Wall -Werror)
endif()

if (LINUX)
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined")
endif ()

add_subdirectory(src)
if (BUILD_DOCS)
    add_subdirectory(docs)
endif ()

if (HDMAYA_BUILD_TESTS)
    # Enable CTest - for some odd reason, this doesn't work if called
    # from tests/CMakeLists.txt, even if it's the first thing
    enable_testing()
    add_subdirectory(tests)
endif()

install(
    FILES README.md
    DESTINATION .)

install(
    FILES LICENSE.txt
    DESTINATION .)