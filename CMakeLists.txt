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

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    if (WIN32)
        message(FATAL_ERROR "Compiler does not support 64-bit builds. "
                "If you are using Visual Studio, make sure you are in the "
                "x64 command prompt and have specified the Win64 cmake "
                "generator (e.g., -G \"Visual Studio 14 2015 Win64\")")
    else()
        message(FATAL_ERROR "Compiler does not support 64-bit builds.")
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/defaults
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)

include(Options)

# Versions prior to 3.1.0 on Unix platforms built archive libraries
# incorrectly.  Require at least that version if necessary.
if((NOT BUILD_SHARED_LIBS OR PXR_BUILD_MONOLITHIC) AND NOT WIN32)
    cmake_minimum_required(VERSION 3.1.0)
endif()

include(ProjectDefaults)

# CXXDefaults will set a variety of variables for the project.
# Consume them here. This is an effort to keep the most common
# build files readable.
include(CXXDefaults)
add_definitions(${_PXR_CXX_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${_PXR_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

include(Public)

find_package(Maya REQUIRED)
find_package(USD REQUIRED)

#==============================================================================
# PACKAGES USED WHILE BUILING
#==============================================================================

# Used to build the Python bindings of the SDK
if(NOT(PYTHON_INCLUDE_DIRS AND PYTHON_LIBRARIES))
	include(cmake/python.cmake)
endif()
include_directories(${PYTHON_INCLUDE_DIRS})

#==============================================================================
# AL_USDMAYA CMAKE VARS
#==============================================================================

set(CMAKE_MODULE_PATH     "${CMAKE_SOURCE_DIR}/al_usdmaya/cmake/modules")
set(MAYA_LOCATION         "${MAYA_LOCATION}")
set(NO_TESTS 1 )
set(SKIP_USDMAYA_TESTS ON CACHE BOOL "Skip tests")
set(BOOST_LIBRARYDIR "${USD_ROOT}/lib" )
if (WIN32)
	set(BOOST_INCLUDEDIR "${USD_ROOT}/include/boost-1_61" )
else()
	set(BOOST_INCLUDEDIR "${USD_ROOT}/include" )
endif()
set(BOOST_ROOT "${USD_ROOT}" )

#add_definitions(-DWANT_UFE_BUILD)
#set(CMAKE_WANT_UFE_BUILD "1")

add_subdirectory(AL_USDMaya)
