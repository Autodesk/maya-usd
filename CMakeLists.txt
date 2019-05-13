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

set(CMAKE_WANT_UFE_BUILD ON)

#==============================================================================
# Modules
#==============================================================================
if (BUILD_MAYAUSD_CORE)
  include(cmake/mayausd_version.info)
  set(MAYAUSD_VERSION "${MAYAUSD_MAJOR_VERSION}.${MAYAUSD_MINOR_VERSION}.${MAYAUSD_PATCH_LEVEL}")
endif()

include(cmake/utils.cmake)
include(cmake/usd.cmake)
include(cmake/ufe.cmake)

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/defaults
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)

#==============================================================================
# 
#==============================================================================
set(USD_CONFIG_FILE  "${PXR_USD_LOCATION}/cmake/pxrConfig.cmake")
set(BOOST_LIBRARYDIR "${PXR_USD_LIB_DIR}" )
set(BOOST_ROOT       "${PXR_USD_LOCATION}" )

#==============================================================================
# Build Core
#==============================================================================
if (BUILD_MAYAUSD_CORE)
    # Including this file will end up calling ExternalProjet_Add which we use to
    # perform the MayaUSD build. Note: That external project is executed immediately
    # during the 'configure' phase of the AL_USDMaya cmake. This means that MayaUSD
    # will perform the cmake config/build/install steps all at once. This is needed
    # because AL_USDMaya needs to find an 'install' of MayaUSD.

    set(MAYAUSD_BUILD_ROOT "${CMAKE_CURRENT_BINARY_DIR}/build-mayaUsd")

    include("Core/MayaUsd.cmake")
endif()

#==============================================================================
# PLUGINS
#==============================================================================
