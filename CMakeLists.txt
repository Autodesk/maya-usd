#
# =======================================================================
# Copyright 2019 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# =======================================================================
#

cmake_minimum_required(VERSION 3.1.1)

project(maya-usd)

#==============================================================================
# Define common build variables
#==============================================================================
option(BUILD_MAYAUSD_LIBRARY "Build Core USD libraries." ON)
option(BUILD_ADSK_PLUGIN "Build Autodesk USD plugin." ON)
option(BUILD_PXR_PLUGIN "Build the Pixar USD plugin and libraries." ON)
option(BUILD_AL_PLUGIN "Build the Animal Logic USD plugin and libraries." ON)
option(BUILD_TESTS "Build tests." ON)
option(WANT_USD_RELATIVE_PATH "Use relative rpaths for USD libraries" OFF)
option(CMAKE_WANT_UFE_BUILD "Enable building with UFE (if found)." ON)

# Avoid noisy install messages
set(CMAKE_INSTALL_MESSAGE "NEVER")
#==============================================================================
# Modules and Definitions
#==============================================================================
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/defaults
)

if (BUILD_MAYAUSD_LIBRARY)
    include(cmake/mayausd_version.info)
    set(MAYAUSD_VERSION "${MAYAUSD_MAJOR_VERSION}.${MAYAUSD_MINOR_VERSION}.${MAYAUSD_PATCH_LEVEL}")
endif()

if (APPLE)
   set(CMAKE_OSX_ARCHITECTURES "x86_64")
endif()

if (DEFINED PYTHON_INCLUDE_DIR AND DEFINED PYTHON_LIBRARIES AND DEFINED PYTHON_EXECUTABLE)
    SET(PYTHON_INCLUDE_DIRS "${PYTHON_INCLUDE_DIR}")
    SET(PYTHONLIBS_FOUND TRUE)
    find_package(PythonInterp 2.7 REQUIRED)
    if(NOT PYTHONINTERP_FOUND)
        set(PYTHONLIBS_FOUND FALSE)
    endif()
endif()
if (NOT PYTHONLIBS_FOUND)
    include(cmake/python.cmake)
endif()
include(cmake/utils.cmake)

find_package(Maya REQUIRED)
find_package(USD REQUIRED)
include(cmake/usd.cmake)
include(${USD_CONFIG_FILE})

if(CMAKE_WANT_UFE_BUILD)
    find_package(UFE QUIET)
    if(UFE_FOUND)
        message(STATUS "Building with UFE ${UFE_VERSION} features enabled.")
    else()
        message(STATUS "UFE not found. UFE features will be disabled.")
    endif()
endif()

#==============================================================================
# Compiler
#==============================================================================
# CXXDefaults will set a variety of variables for the project.
# Consume them here. This is an effort to keep the most common
# build files readable.
include(CXXDefaults)
add_definitions(${_PXR_CXX_DEFINITIONS} -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
set(CMAKE_CXX_FLAGS "${_PXR_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS
        -msse3
        "${CMAKE_CXX_FLAGS} ${_PXR_CXX_FLAGS}"
    )
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")


#==============================================================================
# Tests
#==============================================================================
if (BUILD_TESTS)
    include(cmake/Googletest.cmake)
    fetch_googletest()

    enable_testing()
    if (UFE_FOUND)
        add_subdirectory(test/lib/ufe)
    endif()
endif()

#==============================================================================
# CORE
#==============================================================================
if (BUILD_MAYAUSD_LIBRARY)
    add_subdirectory(lib)
endif()

#==============================================================================
# PLUGINS
#==============================================================================
if (BUILD_PXR_PLUGIN)
    add_subdirectory(plugin/pxr)
endif()

if (BUILD_AL_PLUGIN)
    if(BUILD_USDMAYA_PXR_TRANSLATORS AND NOT BUILD_PXR_PLUGIN)
        message(FATAL_ERROR "Animal logic's pixar translator depends on pixar plugin!")
    endif()

    add_subdirectory(plugin/al)

    if(BUILD_USDMAYA_PXR_TRANSLATORS)
        add_dependencies(AL_USDMayaPxrTranslators usdMaya)
    endif()
endif()

if (BUILD_ADSK_PLUGIN)
    add_subdirectory(plugin/adsk)
endif()

#==============================================================================
# Install mayaUSD.mod
#==============================================================================
configure_file(mayaUSD.mod.template ${PROJECT_BINARY_DIR}/mayaUSD.mod)
install(FILES ${PROJECT_BINARY_DIR}/mayaUSD.mod
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)

