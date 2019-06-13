#-
#*****************************************************************************
# Copyright 2018 Autodesk, Inc. All rights reserved.
#
# These coded instructions, statements, and computer programs contain
# unpublished proprietary information written by Autodesk, Inc. and are
# protected by Federal copyright law. They may not be disclosed to third
# parties or copied or duplicated in any form, in whole or in part, without
# the prior written consent of Autodesk, Inc.
#*****************************************************************************
#+

if (WIN32)
    cmake_minimum_required(VERSION 3.1.1)
else()
    cmake_minimum_required(VERSION 2.8.8)
endif()

#==============================================================================
# Define common build variables
#==============================================================================

set(BUILD_MAYAUSD_CORE ON CACHE BOOL "Build the Maya USD core libraries.")
set(BUILD_MAYAUSD_PLUGIN ON CACHE BOOL "Build the Maya USD plugin.")
set(BUILD_AL_USD OFF CACHE BOOL "Build the Animal Logic plugin and libraries.")

#==============================================================================
# Modules and Definitions
#==============================================================================
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/defaults
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)

if (BUILD_MAYAUSD_CORE)
    include(cmake/mayausd_version.info)
    set(MAYAUSD_VERSION "${MAYAUSD_MAJOR_VERSION}.${MAYAUSD_MINOR_VERSION}.${MAYAUSD_PATCH_LEVEL}")
endif()

if (APPLE)
   set(CMAKE_MACOSX_RPATH ON)
   add_definitions(-DOSMac_ -DMAC_PLUGIN)
   add_definitions(-D_BOOL -DREQUIRE_IOSTREAM)
endif()

include(cmake/python.cmake)
include(cmake/utils.cmake)
include(cmake/usd.cmake)

#==============================================================================
# 
#==============================================================================
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WIN32)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} ${_PXR_CXX_FLAGS}"
    )
else()
    set(CMAKE_CXX_FLAGS
        -std=c++11
        -msse3
        "${CMAKE_CXX_FLAGS} ${_PXR_CXX_FLAGS}"
    )
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")


#==============================================================================
# Build Core
#==============================================================================
if (BUILD_MAYAUSD_CORE)

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
if (PXR_BUILD_MAYA_PLUGIN)
    add_subdirectory(plugins/USD)
endif()

if (BUILD_AL_USD)
    add_subdirectory(plugins/AL_USDMaya)
endif()

if (BUILD_MAYAUSD_PLUGIN)
    add_subdirectory(plugins/mayaUSD)
endif()
