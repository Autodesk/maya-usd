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
project(mayaUsd)

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 OLD)
endif()

#==============================================================================
# Modules and Definitions
#==============================================================================
list(APPEND CMAKE_MODULE_PATH
    ${PROJECT_SOURCE_DIR}/../cmake/defaults
    ${PROJECT_SOURCE_DIR}/../cmake/modules
)

if (WIN32)
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

if (APPLE)
   set(CMAKE_MACOSX_RPATH ON)
endif()

#==============================================================================
# Packages
#==============================================================================
find_package(Maya REQUIRED)

add_subdirectory(include)
add_subdirectory(src)