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

cmake_minimum_required(VERSION 3.1.1)

#==============================================================================
# Define common build variables
#==============================================================================

set(BUILD_MAYAUSD_CORE ON CACHE BOOL "Build the Maya USD core libraries.")
set(BUILD_MAYAUSD_PLUGIN ON CACHE BOOL "Build the Maya USD plugin.")
set(BUILD_AL_USD OFF CACHE BOOL "Build the Animal Logic plugin and libraries.")
set(BUILD_PXR_USD OFF CACHE BOOL "Build the Pixar plugin and libraries.")

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

include(cmake/python.cmake)
include(cmake/utils.cmake)
include(cmake/usd.cmake)

#==============================================================================
# 
#==============================================================================
set(BOOST_LIBRARYDIR "${PXR_USD_LIB_DIR}" )
set(BOOST_ROOT       "${PXR_USD_LOCATION}" )

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
if (BUILD_PXR_USD)
    add_subdirectory(plugins/USD)
endif()

if (BUILD_AL_USD)
    add_subdirectory(plugins/AL_USDMaya)
endif()

if (BUILD_MAYAUSD_PLUGIN)
    add_subdirectory(plugins/mayaUSD)
endif()
