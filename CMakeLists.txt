#-
# =======================================================================
# Copyright 2019 Autodesk, Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# =======================================================================
#+

cmake_minimum_required(VERSION 3.1.1)

#==============================================================================
# Define common build variables
#==============================================================================
set(BUILD_PXR_PLUGIN ON CACHE BOOL "Build the Pixar USD plugin and libraries.")
set(BUILD_AL_PLUGIN ON CACHE BOOL "Build the Animal Logic USD plugin and libraries.")

#==============================================================================
# Modules and Definitions
#==============================================================================
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/defaults
)

if (APPLE)
   set(OSX_ARCHITECTURES "x86_64")
   add_definitions(-DOSMac_ -DMAC_PLUGIN)
   add_definitions(-D_BOOL -DREQUIRE_IOSTREAM)
endif()

include(cmake/python.cmake)
include(cmake/utils.cmake)

find_package(USD REQUIRED)
include(cmake/usd.cmake)

#==============================================================================
# Compiler
#==============================================================================
# CXXDefaults will set a variety of variables for the project.
# Consume them here. This is an effort to keep the most common
# build files readable.
include(CXXDefaults)
add_definitions(${_PXR_CXX_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${_PXR_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS
        -msse3
        "${CMAKE_CXX_FLAGS} ${_PXR_CXX_FLAGS}"
    )
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")


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


#==============================================================================
# Install mayaUSD.mod
#==============================================================================
configure_file(mayaUSD.mod.template ${PROJECT_BINARY_DIR}/mayaUSD.mod)
install(FILES ${PROJECT_BINARY_DIR}/mayaUSD.mod
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
