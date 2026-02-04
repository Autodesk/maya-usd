#
# Copyright 2023 Autodesk
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
option(BUILD_MAYAUSD_LIBRARY "Build Core USD libraries." ON)
option(BUILD_MAYAUSDAPI_LIBRARY "Build the mayaUsdAPI subset library that provides a stable versioned interface to mayaUsd for external plugins." ON)
option(BUILD_LOOKDEVXUSD_LIBRARY "Build LookdevXUsd library using LookdevXUfe." ON)
option(BUILD_ADSK_PLUGIN "Build Autodesk USD plugin." ON)
option(BUILD_PXR_PLUGIN "Build the Pixar USD plugin and libraries." ON)
option(BUILD_AL_PLUGIN "Build the Animal Logic USD plugin and libraries." ON)
option(BUILD_HDMAYA "Build the legacy Maya-To-Hydra plugin and scene delegate." OFF)
option(BUILD_RFM_TRANSLATORS "Build translators for RenderMan for Maya shaders." ON)
option(BUILD_TESTS "Build tests." ON)
option(BUILD_STRICT_MODE "Enforce all warnings as errors." ON)
option(BUILD_SHARED_LIBS "Build libraries as shared or static." ON)
option(BUILD_WITH_PYTHON_3 "Build with python 3." OFF)
if(APPLE)
    option(BUILD_UB2 "Build Universal Binary 2 (UB2) Intel64+arm64" OFF)
endif()
set(BUILD_WITH_PYTHON_3_VERSION 3.7 CACHE STRING "The version of Python 3 to build with")
option(CMAKE_WANT_MATERIALX_BUILD "Enable building with MaterialX." ON)
option(CMAKE_WANT_VALIDATE_UNDO_ITEM "Enable validating undo items list." OFF)

set(PXR_OVERRIDE_PLUGINPATH_NAME PXR_PLUGINPATH_NAME
    CACHE STRING "Name of env var USD searches to find plugins")

# Build-related information, expected to be passed in by the parent build.

# Build number corresponding to this build
#
# This is used to report version information about the build number that
# has produced this binary. When unset, the default value "0" is used
# instead as this usually corresponds to developers local builds.
set(MAYAUSD_BUILD_NUMBER "0" CACHE STRING "Build number corresponding to this build.")

# Git commit hash corresponding to this build
#
# This is used to report version information of the Git commit has that
# corresponds to a build. When unset, the default value DEVBLD is used
# instead as this usually corresponds to a developers local build.
set(MAYAUSD_GIT_COMMIT "DEVBLD" CACHE STRING "Git commit hash corresponding to this build.")

# Git branch corresponding to this build
#
# This is used to report version information about the Git branch of a
# build. When unset, the default value DEVBR is used instead as this
# usually corresponds to developers local builds.
set(MAYAUSD_GIT_BRANCH "DEVBR" CACHE STRING "Change set identifier corresponding to this build.")

# MayaUsd cut-id

# This is used to distinguish daily builds from one another. It is a
# usually a combination of the date & git commit hash.
set(MAYAUSD_CUT_ID "DEVBLD" CACHE STRING "MayaUsd cut-id corresponding to this build.")

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
if (BUILD_MAYAUSD_LIBRARY)
    find_package(Maya 2022 REQUIRED)
endif()

if(APPLE AND BUILD_UB2 AND NOT MAYA_MACOSX_BUILT_WITH_UB2)
    message(WARNING  "Maya was NOT built with Universal Binary 2")
endif()

if (BUILD_MAYAUSD_LIBRARY)
    include(cmake/mayausd_version.info)
    set(MAYAUSD_VERSION "${MAYAUSD_MAJOR_VERSION}.${MAYAUSD_MINOR_VERSION}.${MAYAUSD_PATCH_LEVEL}")
endif()

if (BUILD_MAYAUSDAPI_LIBRARY)
    include(cmake/mayausdapi_version.info)
    set(MAYAUSDAPI_VERSION "${MAYAUSDAPI_MAJOR_VERSION}.${MAYAUSDAPI_MINOR_VERSION}.${MAYAUSDAPI_PATCH_LEVEL}")
endif()

if (BUILD_MAYAUSDAPI_LIBRARY)
    if (NOT BUILD_MAYAUSD_LIBRARY)
        message(FATAL_ERROR "Building mayaUsdAPI library requires MayaUsd core libraries to be built, please enable BUILD_MAYAUSD_LIBRARY.")
    endif()
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
message(STATUS "Build MayaUSD with Python3 = " ${BUILD_WITH_PYTHON_3})
message(STATUS "   PYTHON_INCLUDE_DIRS = ${PYTHON_INCLUDE_DIRS}")
message(STATUS "   PYTHON_LIBRARIES    = ${PYTHON_LIBRARIES}")
message(STATUS "   Python_EXECUTABLE   = ${Python_EXECUTABLE}")

if (BUILD_MAYAUSD_LIBRARY)
    include(cmake/jinja.cmake)
endif()

find_package(USD 0.21.11 REQUIRED)
if (BUILD_MAYAUSD_LIBRARY OR BUILD_AL_PLUGIN OR BUILD_TESTS)
    if(NOT USD_GENSCHEMA)
        message(FATAL_ERROR "Building MayaUsd requires usdGenSchema (from USD)")
    endif()
endif()
if (CMAKE_WANT_MATERIALX_BUILD)
    # Requires at least USD 21.08 for hdMtlx module and USD must have been built with MaterialX:
    if(NOT TARGET hdMtlx)
        set(CMAKE_WANT_MATERIALX_BUILD OFF)
        message(WARNING "Disabling MaterialX VP2 rendering: it is not supported by this USD package.")
    endif()
endif()
include(cmake/usd.cmake)

find_package(UFE 0.2.0 REQUIRED)
message(STATUS "Build MayaUsd with UFE version ${UFE_VERSION}")
message(STATUS "   UFE include dir: ${UFE_INCLUDE_DIR}")
message(STATUS "   UFE library: ${UFE_LIBRARY}")
message(STATUS "   UFE preview features: ${UFE_PREVIEW_FEATURES}")

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

# Only need Qt when building MayaUsd (i.e. Qt not needed for UsdUfe standalone build).
if(BUILD_MAYAUSD_LIBRARY)
    if(MAYA_APP_VERSION VERSION_GREATER 2024)
        # First look for Qt6 in the Maya devkit.
        # The Qt6 archive in the Maya devkit contains everything needed for the normal cmake find_package.
        set(CMAKE_PREFIX_PATH "${MAYA_DEVKIT_LOCATION}/Qt")
        find_package(Qt6 6.5 COMPONENTS Core Gui Widgets QUIET)
    endif()
    if (Qt6_FOUND)
        message(STATUS "Found Qt6 in Maya devkit, building with Qt ${Qt6_VERSION} features enabled.")
        set(Qt_FOUND TRUE)
    else()
        # If we didn't find Qt6 in Maya devkit, search again, but for Qt5 this time.
        # Minimum version required is 5.15 (Maya 2022/2023/2024).
        # This will find Qt in the Maya devkit using a custom find package.
        # So the version will match Maya we are building for.
        find_package(Maya_Qt 5.15 COMPONENTS Core Gui Widgets QUIET)
        if (Maya_Qt_FOUND)
            message(STATUS "Found Qt5 in Maya devkit, building with Qt ${MAYA_QT_VERSION} features enabled.")
            set(Qt_FOUND TRUE)
        endif()
    endif()

    if(NOT Qt_FOUND)
        message(SEND_ERROR "Could not find Qt in Maya devkit directory: ${MAYA_DEVKIT_LOCATION}.")
        if(MAYA_APP_VERSION VERSION_GREATER 2024)
            message(STATUS "  You must extract Qt.tar.gz")
        else()
            if (IS_WINDOWS)
                message(STATUS "  In Maya devkit you must extract include/qt_5.15.2_vc14-include.zip")
            else()
                message(STATUS "  In Maya devkit you must extract include/qt_5.15.2-include.tar.gz")
            endif()
        endif()
        message(FATAL_ERROR "Cannot build MayaUsd without Qt.")
    endif()
endif()

if (NOT BUILD_ADSK_PLUGIN)
        set(BUILD_LOOKDEVXUSD_LIBRARY OFF)
endif()
# Need LookdevXUfe/MayaUsdAPI when building LookdevXUsd.
if(BUILD_LOOKDEVXUSD_LIBRARY)
    if(NOT BUILD_MAYAUSDAPI_LIBRARY)
        message(FATAL_ERROR "Building LookdevXUsd requires MayaUsdAPI.")
    endif()

    if(MAYA_APP_VERSION VERSION_GREATER 2025)
        find_package(LookdevXUfe) # Optional component - if not found, disable LookdevXUsd.
    else()
        set(BUILD_LOOKDEVXUSD_LIBRARY OFF)
        message(WARNING "Disabling LookdevXUsd: it is not supported by Maya ${MAYA_APP_VERSION}.")
    endif()
    if (LookdevXUfe_FOUND)
        message(STATUS "Build LookdevXUsd with LookdevXUfe version: ${LookdevXUfe_VERSION}")
        message(STATUS "   LookdevXUfe include dir: ${LookdevXUfe_INCLUDE_DIR}")
        message(STATUS "   LookdevXUfe library    : ${LookdevXUfe_LIBRARY}")
    else()
        set(BUILD_LOOKDEVXUSD_LIBRARY OFF)
        message(WARNING "Disabling LookdevXUsd as LookdevXUfe was not found (in Maya devkit).")
    endif()
endif()

find_package(AdskUsdComponentCreator)
find_package(AR)
find_package(UsdEditForward)

#------------------------------------------------------------------------------
# compiler configuration
#------------------------------------------------------------------------------
include(cmake/compiler_config.cmake)

#------------------------------------------------------------------------------
# C++17 support
#------------------------------------------------------------------------------
# In cmake/compiler_config.cmake we set the C++ version to 17 when building with
# USD 23.11 or greater to match USD.
# Some features also require C++17 so disable them if they were enabled.
if (USD_VERSION VERSION_LESS "0.23.11")
    # The clipboard support was added in Ufe v6 and backported to Ufe v5.
    # Compiling UsdUfe standalone with Ufe v5 will result in clipboard being
    # enabled, but we must disable it here if USD version doesn't match C++ version.
    if (UFE_CLIPBOARD_SUPPORT)
        message(WARNING "Disabling Ufe clipboard support since it requires USD v23.11 and C++17")
        set(UFE_CLIPBOARD_SUPPORT FALSE)
    endif()
endif()

#------------------------------------------------------------------------------
# gulrak filesystem
#------------------------------------------------------------------------------
if (BUILD_MAYAUSD_LIBRARY OR BUILD_AL_PLUGIN)
    include(cmake/gulrak.cmake)
endif()

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
# MaterialX Python bindings
#------------------------------------------------------------------------------
if (USD_VERSION VERSION_LESS "0.23.11")
    set(MAYAUSD_EXTRA_PYTHON_PATH "")
else()
    set(MAYAUSD_EXTRA_PYTHON_PATH "PYTHONPATH+:=python")
endif()

#------------------------------------------------------------------------------
# lib
#------------------------------------------------------------------------------
add_subdirectory(lib)

#------------------------------------------------------------------------------
# tutorials
#------------------------------------------------------------------------------
if (BUILD_MAYAUSD_LIBRARY)
    add_subdirectory(tutorials/import-export-plugin-c++)
endif()

#------------------------------------------------------------------------------
# plugins
#------------------------------------------------------------------------------
if (BUILD_PXR_PLUGIN OR BUILD_AL_PLUGIN OR BUILD_ADSK_PLUGIN)
    if (NOT BUILD_MAYAUSD_LIBRARY)
        message(FATAL_ERROR "Building plugins requires MayaUsd core libraries.")
    endif()
endif()

if (BUILD_PXR_PLUGIN)
    add_subdirectory(plugin/pxr)
endif()

if (BUILD_AL_PLUGIN)
    if(BUILD_USDMAYA_PXR_TRANSLATORS AND NOT BUILD_PXR_PLUGIN)
        message(FATAL_ERROR "Animal logic's pixar translator depends on pixar plugin!")
    endif()

    add_subdirectory(plugin/al)
endif()

if (BUILD_ADSK_PLUGIN)
    add_subdirectory(plugin/adsk)
endif()

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
if (BUILD_ADSK_PLUGIN)
    if (IS_WINDOWS)
        configure_file("modules/mayaUSD_Win.mod.template" ${PROJECT_BINARY_DIR}/mayaUSD.mod)
    else()
        configure_file("modules/mayaUSD.mod.template" ${PROJECT_BINARY_DIR}/mayaUSD.mod)
    endif()
    install(FILES ${PROJECT_BINARY_DIR}/mayaUSD.mod DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

if (BUILD_HDMAYA)
    configure_file("modules/mtoh.mod.template" ${PROJECT_BINARY_DIR}/mtoh.mod)
    install(FILES ${PROJECT_BINARY_DIR}/mtoh.mod DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

if (BUILD_PXR_PLUGIN)
    if (IS_WINDOWS)
        configure_file("modules/pxrUSD_Win.mod.template" ${PROJECT_BINARY_DIR}/pxrUSD.mod)
    else()
        configure_file("modules/pxrUSD.mod.template" ${PROJECT_BINARY_DIR}/pxrUSD.mod)
    endif()
    install(FILES ${PROJECT_BINARY_DIR}/pxrUSD.mod DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

if (BUILD_AL_PLUGIN)
    if (IS_WINDOWS)
        configure_file("modules/alUSD_Win.mod.template" ${PROJECT_BINARY_DIR}/alUSD.mod)
    else()
        configure_file("modules/alUSD.mod.template" ${PROJECT_BINARY_DIR}/alUSD.mod)
    endif()
    install(FILES ${PROJECT_BINARY_DIR}/alUSD.mod DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()


#------------------------------------------------------------------------------
# Special file for handling boost vs pxr::boost
#------------------------------------------------------------------------------
set(TOP_HEADERS
    cxx17_legacy_support.h
    pxr_python.h
)

mayaUsd_promoteHeaderList(HEADERS ${TOP_HEADERS} BASEDIR "NONE")
install(FILES ${CMAKE_BINARY_DIR}/include/pxr_python.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)
install(FILES ${CMAKE_BINARY_DIR}/include/cxx17_legacy_support.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)
