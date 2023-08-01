#
# Copyright 2020 Autodesk
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
#
cmake_minimum_required(VERSION 3.13...3.17)

project(maya-usd)

#------------------------------------------------------------------------------
# options
#------------------------------------------------------------------------------
option(BUILD_TESTS "Build tests." ON)
option(BUILD_STRICT_MODE "Enforce all warnings as errors." ON)
option(BUILD_SHARED_LIBS "Build libraries as shared or static." ON)
option(BUILD_WITH_PYTHON_3 "Build with python 3." OFF)
if(APPLE)
    option(BUILD_UB2 "Build Universal Binary 2 (UB2) Intel64+arm64" OFF)
endif()
set(BUILD_WITH_PYTHON_3_VERSION 3.7 CACHE STRING "The version of Python 3 to build with")
option(CMAKE_WANT_MATERIALX_BUILD "Enable building with MaterialX (experimental)." OFF)

set(PXR_OVERRIDE_PLUGINPATH_NAME PXR_PLUGINPATH_NAME
    CACHE STRING "Name of env var USD searches to find plugins")

#------------------------------------------------------------------------------
# internal flags to control build
#------------------------------------------------------------------------------
# MAYAUSD_TO_USD_RELATIVE_PATH : Set this variable to any relative path from install
#                                folder to USD location. If defined will set relative
#                                rpaths for USD libraries.

#------------------------------------------------------------------------------
# global options
#------------------------------------------------------------------------------
# Avoid noisy install messages
set(CMAKE_INSTALL_MESSAGE "NEVER")

if(APPLE)
    if(BUILD_UB2)
        message(STATUS "Building with Universal Binary 2")
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
        set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
    else()
        set(CMAKE_OSX_ARCHITECTURES "x86_64")
        set(CMAKE_OSX_DEPLOYMENT_TARGET 10.14)
    endif()
endif()

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)

# Use RUNPATH instead of RPATH for all shared libs, executables and modules on Linux
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux") # IS_LINUX not yet defined
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-new-dtags")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-new-dtags")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-new-dtags")
endif()

#------------------------------------------------------------------------------
# modules and definitions
#------------------------------------------------------------------------------
include(cmake/utils.cmake)
find_package(Maya 2018 REQUIRED)

if(APPLE AND BUILD_UB2 AND NOT MAYA_MACOSX_BUILT_WITH_UB2)
    message(WARNING  "Maya was NOT built with Universal Binary 2")
endif()

include(cmake/mayahydra_version.info)
set(MAYAHYDRA_VERSION "${MAYAHYDRA_MAJOR_VERSION}.${MAYAHYDRA_MINOR_VERSION}.${MAYAHYDRA_PATCH_LEVEL}")
if(DEFINED ENV{PLUGIN_CUT_ID})
    set(MAYAHYDRA_CUT_ID $ENV{PLUGIN_CUT_ID})
endif()

if (DEFINED PYTHON_INCLUDE_DIR AND DEFINED PYTHON_LIBRARIES AND DEFINED Python_EXECUTABLE)
    SET(PYTHON_INCLUDE_DIRS "${PYTHON_INCLUDE_DIR}")
    SET(PYTHONLIBS_FOUND TRUE)
    # Use the Python module to find the python lib.
    if(BUILD_WITH_PYTHON_3)
        find_package(Python ${BUILD_WITH_PYTHON_3_VERSION} EXACT REQUIRED COMPONENTS Interpreter)
    else()
        find_package(Python 2.7 EXACT REQUIRED COMPONENTS Interpreter)
    endif()
    if(NOT Python_Interpreter_FOUND)
        set(PYTHONLIBS_FOUND FALSE)
    endif()
endif()
if (NOT PYTHONLIBS_FOUND)
    include(cmake/python.cmake)
endif()
message(STATUS "Build mayaHydra with Python3 = " ${BUILD_WITH_PYTHON_3})
message(STATUS "   PYTHON_INCLUDE_DIRS = ${PYTHON_INCLUDE_DIRS}")
message(STATUS "   PYTHON_LIBRARIES    = ${PYTHON_LIBRARIES}")
message(STATUS "   Python_EXECUTABLE   = ${Python_EXECUTABLE}")

include(cmake/jinja.cmake)

find_package(USD 0.20.05 REQUIRED)
if (CMAKE_WANT_MATERIALX_BUILD)
    # Requires at least USD 21.08 for hdMtlx module and USD must have been built with MaterialX:
    if(NOT TARGET hdMtlx)
        set(CMAKE_WANT_MATERIALX_BUILD OFF)
        message(WARNING "Disabling MaterialX VP2 rendering: it is not supported by this USD package.")
    endif()
endif()
include(cmake/usd.cmake)

option(BUILD_MAYAHYDRALIB "Build the Maya-To-Hydra plugin and scene delegate." ON)

find_package(UFE REQUIRED)
if(UFE_FOUND)
    message(STATUS "Building with UFE ${UFE_VERSION}.")
else()
    message(STATUS "UFE not found.")
endif()

if(CMAKE_WANT_MATERIALX_BUILD AND MAYA_LIGHTAPI_VERSION LESS 2)
    set(CMAKE_WANT_MATERIALX_BUILD OFF)
    message(WARNING "Disabling MaterialX VP2 rendering: it is not supported by this version of Maya.")
endif()

if(TARGET hdMtlx)
    # MaterialX was built into USD. We need to know where to find MaterialX targets.
    find_package(MaterialX REQUIRED)
    if(TARGET MaterialXCore)
        message(STATUS "Found MaterialX")
    endif()
endif()

#------------------------------------------------------------------------------
# compiler configuration
#------------------------------------------------------------------------------
include(cmake/compiler_config.cmake)

#------------------------------------------------------------------------------
# gulrak filesystem
#------------------------------------------------------------------------------
include(cmake/gulrak.cmake)

#------------------------------------------------------------------------------
# test
#------------------------------------------------------------------------------
if (BUILD_TESTS)
    include(cmake/googletest.cmake)
    include(cmake/test.cmake)
    fetch_googletest()
    enable_testing()
    add_subdirectory(test)
    set(MAYAUSD_GTEST_PATH "PATH+:=lib/gtest")
endif()

#------------------------------------------------------------------------------
# lib
#------------------------------------------------------------------------------
add_subdirectory(lib)

#------------------------------------------------------------------------------
# install
#------------------------------------------------------------------------------
if (DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
    set(USD_INSTALL_LOCATION "${CMAKE_INSTALL_PREFIX}/${MAYAUSD_TO_USD_RELATIVE_PATH}")
else()
    set(USD_INSTALL_LOCATION ${PXR_USD_LOCATION})
endif()


#------------------------------------------------------------------------------
# Maya module files
#------------------------------------------------------------------------------
if (IS_WINDOWS)
    configure_file("modules/mayaHydra_Win.mod.template" ${PROJECT_BINARY_DIR}/mayaHydra.mod)
else()
    configure_file("modules/mayaHydra.mod.template" ${PROJECT_BINARY_DIR}/mayaHydra.mod)
endif()
install(FILES ${PROJECT_BINARY_DIR}/mayaHydra.mod DESTINATION ${CMAKE_INSTALL_PREFIX})
