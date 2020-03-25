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
option(BUILD_MAYAUSD_LIBRARY "Build Core USD libraries." ON)
option(BUILD_ADSK_PLUGIN "Build Autodesk USD plugin." ON)
option(BUILD_PXR_PLUGIN "Build the Pixar USD plugin and libraries." ON)
option(BUILD_AL_PLUGIN "Build the Animal Logic USD plugin and libraries." ON)
option(BUILD_HDMAYA "Build the Maya-To-Hydra plugin and scene delegate." ON)
option(BUILD_TESTS "Build tests." ON)
option(CMAKE_WANT_UFE_BUILD "Enable building with UFE (if found)." ON)

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

set(CMAKE_OSX_ARCHITECTURES "x86_64")

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/defaults
)

# Use RUNPATH instead of RPATH for all shared libs and executables on Linux
if(IS_LINUX)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-new-dtags")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-new-dtags")
endif()

#------------------------------------------------------------------------------
# modules and definitions
#------------------------------------------------------------------------------
if (BUILD_MAYAUSD_LIBRARY)
    include(cmake/mayausd_version.info)
    set(MAYAUSD_VERSION "${MAYAUSD_MAJOR_VERSION}.${MAYAUSD_MINOR_VERSION}.${MAYAUSD_PATCH_LEVEL}")
endif()

if (DEFINED PYTHON_INCLUDE_DIR AND DEFINED PYTHON_LIBRARIES AND DEFINED Python_EXECUTABLE)
    SET(PYTHON_INCLUDE_DIRS "${PYTHON_INCLUDE_DIR}")
    SET(PYTHONLIBS_FOUND TRUE)
    find_package(Python 2.7 EXACT REQUIRED COMPONENTS Interpreter)
    if(NOT Python_Interpreter_FOUND)
        set(PYTHONLIBS_FOUND FALSE)
    endif()
endif()
if (NOT PYTHONLIBS_FOUND)
    include(cmake/python.cmake)
endif()
include(cmake/utils.cmake)
message(STATUS "Building with Python:")
message(STATUS "   PYTHON_INCLUDE_DIRS = ${PYTHON_INCLUDE_DIRS}")
message(STATUS "   PYTHON_LIBRARIES    = ${PYTHON_LIBRARIES}")
message(STATUS "   Python_EXECUTABLE   = ${Python_EXECUTABLE}")

# The building of the schemas requires some extra python packages which may or
# may not be installed. If they aren't we provide an option to add them from
# a python egg.
if (BUILD_USDMAYA_SCHEMAS)
    include(cmake/jinja.cmake)
endif()

find_package(Maya REQUIRED)
find_package(USD REQUIRED)
include(cmake/usd.cmake)
include(${USD_CONFIG_FILE})

if(${MAYA_APP_VERSION} STRLESS "2019" AND CMAKE_WANT_UFE_BUILD)
    set(CMAKE_WANT_UFE_BUILD OFF)
    message(AUTHOR_WARNING "========================================================================================= \
                            UFE is not available in Maya${MAYA_MAJOR_VERSION}. Maya2019 and later are supported only. \
                            ========================================================================================= ")
endif()

if(CMAKE_WANT_UFE_BUILD)
    find_package(UFE QUIET)
    if(UFE_FOUND)
        message(STATUS "Building with UFE ${UFE_VERSION} features enabled.")
    else()
        message(STATUS "UFE not found. UFE features will be disabled.")
    endif()
endif()

if(DEFINED QT_LOCATION)
    if(NOT DEFINED QT_VERSION)
        if(${MAYA_APP_VERSION} STRLESS_EQUAL "2019")
            set(QT_VERSION "5.6")
        else()
            set(QT_VERSION "5.12")
        endif()
    endif()
    set(CMAKE_PREFIX_PATH "${QT_LOCATION}")
    find_package(Qt5 ${QT_VERSION} COMPONENTS Core Gui Widgets REQUIRED)
    if(Qt5_FOUND)
        message(STATUS "Building with Qt features enabled.")
    endif()
else()
    message(STATUS "QT_LOCATION not set. Building Qt features will be disabled.")
endif()

#------------------------------------------------------------------------------
# compiler configuration
#------------------------------------------------------------------------------
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
if (BUILD_MAYAUSD_LIBRARY)
    add_subdirectory(lib)
endif()

#------------------------------------------------------------------------------
# plugins
#------------------------------------------------------------------------------
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

#------------------------------------------------------------------------------
# install
#------------------------------------------------------------------------------
configure_file(mayaUSD.mod.template ${PROJECT_BINARY_DIR}/mayaUSD.mod)
install(FILES ${PROJECT_BINARY_DIR}/mayaUSD.mod
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
