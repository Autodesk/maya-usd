#-
#*****************************************************************************
# Copyright 2019 Autodesk, Inc. All rights reserved.
#
# These coded instructions, statements, and computer programs contain
# unpublished proprietary information written by Autodesk, Inc. and are
# protected by Federal copyright law. They may not be disclosed to third
# parties or copied or duplicated in any form, in whole or in part, without
# the prior written consent of Autodesk, Inc.
#*****************************************************************************
#+

cmake_minimum_required(VERSION 3.1.1)

#==============================================================================
# Define common build variables
#==============================================================================
set(BUILD_CORE_USD_LIBRARY ON CACHE BOOL "Build Core USD libraries.")
set(BUILD_ADSK_USD_PLUGIN ON CACHE BOOL "Build Autodesk USD plugin.")
set(BUILD_PXR_USD_PLUGIN ON CACHE BOOL "Build the Pixar USD plugin and libraries.")
set(BUILD_AL_USD_PLUGIN ON CACHE BOOL "Build the Animal Logic USD plugin and libraries.")

#==============================================================================
# Modules and Definitions
#==============================================================================
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/defaults
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)

if (BUILD_CORE_USD_LIBRARY)
    include(cmake/mayausd_version.info)
    set(MAYAUSD_VERSION "${MAYAUSD_MAJOR_VERSION}.${MAYAUSD_MINOR_VERSION}.${MAYAUSD_PATCH_LEVEL}")
endif()

if (APPLE)
   set(OSX_ARCHITECTURES "x86_64")
   add_definitions(-DOSMac_ -DMAC_PLUGIN)
   add_definitions(-D_BOOL -DREQUIRE_IOSTREAM)
endif()

include(cmake/python.cmake)
include(cmake/utils.cmake)
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
# CORE
#==============================================================================
if (BUILD_CORE_USD_LIBRARY)

    # set build location
    set(MAYAUSD_BUILD_ROOT "${CMAKE_CURRENT_BINARY_DIR}/build-mayaUsd")

    # Including this file will end up calling ExternalProjet_Add which we use to
    # perform the MayaUSD build. Note: That external project is executed immediately
    # during the 'configure' phase of the AL_USDMaya cmake. This means that MayaUSD
    # will perform the cmake config/build/install steps all at once. This is needed
    # because AL_USDMaya needs to find an 'install' of MayaUSD.
    include("core/MayaUsd.cmake")
endif()

#==============================================================================
# PLUGINS
#==============================================================================
if (BUILD_PXR_USD_PLUGIN)
    add_subdirectory(plugins/PXR_USDMaya)
endif()

if (BUILD_AL_USD_PLUGIN)
    add_subdirectory(plugins/AL_USDMaya)
endif()

if (BUILD_ADSK_USD_PLUGIN)
    add_subdirectory(plugins/ADSK_USDMaya)
endif()
