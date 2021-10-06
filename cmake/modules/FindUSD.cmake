# Simple module to find USD.

# On a system with an existing USD /usr/local installation added to the system
# PATH, use of PATHS in find_path incorrectly causes the existing USD
# installation to be found.  As per
# https://cmake.org/cmake/help/v3.4/command/find_path.html
# and
# https://cmake.org/pipermail/cmake/2010-October/040460.html
# HINTS get searched before system paths, which produces the desired result.
find_path(USD_INCLUDE_DIR
    NAMES
        pxr/pxr.h
    HINTS
        ${PXR_USD_LOCATION}
        $ENV{PXR_USD_LOCATION}
    PATH_SUFFIXES
        include
    DOC
        "USD Include directory"
)

find_file(USD_GENSCHEMA
    NAMES
        usdGenSchema
    PATHS
        ${PXR_USD_LOCATION}
        $ENV{PXR_USD_LOCATION}
    PATH_SUFFIXES
        bin
    DOC
        "USD Gen schema application"
)

find_file(USD_CONFIG_FILE
    NAMES 
        pxrConfig.cmake
    PATHS 
        ${PXR_USD_LOCATION}
        $ENV{PXR_USD_LOCATION}
    DOC "USD cmake configuration file"
)

# PXR_USD_LOCATION might have come in as an environment variable, and
# it could also have been a hint-list, so we'll make sure we set it to
# wherever we found pxrConfig, which is always the correct location.
get_filename_component(PXR_USD_LOCATION "${USD_CONFIG_FILE}" DIRECTORY)

include(${USD_CONFIG_FILE})

if(DEFINED PXR_VERSION)
    # Starting in core USD 21.05, pxrConfig.cmake provides the various USD
    # version numbers as CMake variables, in which case PXR_VERSION should have
    # been defined, along with the major, minor, and patch version numbers, so
    # there is no need to extract them from the pxr/pxr.h header file anymore.
    # The only thing we need to do is assemble the USD_VERSION version string.
    set(USD_VERSION ${PXR_MAJOR_VERSION}.${PXR_MINOR_VERSION}.${PXR_PATCH_VERSION})
elseif(USD_INCLUDE_DIR AND EXISTS "${USD_INCLUDE_DIR}/pxr/pxr.h")
    foreach(_usd_comp MAJOR MINOR PATCH)
        file(STRINGS
            "${USD_INCLUDE_DIR}/pxr/pxr.h"
            _usd_tmp
            REGEX "#define PXR_${_usd_comp}_VERSION .*$")
        string(REGEX MATCHALL "[0-9]+" USD_${_usd_comp}_VERSION ${_usd_tmp})
    endforeach()
    set(USD_VERSION ${USD_MAJOR_VERSION}.${USD_MINOR_VERSION}.${USD_PATCH_VERSION})
    math(EXPR PXR_VERSION "${USD_MAJOR_VERSION} * 10000 + ${USD_MINOR_VERSION} * 100 + ${USD_PATCH_VERSION}")
endif()

# Note that on Windows with USD <= 0.19.11, USD_LIB_PREFIX should be left at
# default (or set to empty string), even if PXR_LIB_PREFIX was specified when
# building core USD, due to a bug.

# On all other platforms / versions, it should match the PXR_LIB_PREFIX used
# for building USD (and shouldn't need to be touched if PXR_LIB_PREFIX was not
# used / left at it's default value). Starting with USD 21.11, the default
# value for PXR_LIB_PREFIX was changed to include "usd_".

if (USD_VERSION VERSION_GREATER_EQUAL "0.21.11")
    set(USD_LIB_PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}usd_"
        CACHE STRING "Prefix of USD libraries; generally matches the PXR_LIB_PREFIX used when building core USD")
else()
    set(USD_LIB_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX}
        CACHE STRING "Prefix of USD libraries; generally matches the PXR_LIB_PREFIX used when building core USD")
endif()

if (WIN32)
    # ".lib" on Windows
    set(USD_LIB_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX}
        CACHE STRING "Extension of USD libraries")
else ()
    # ".so" on Linux, ".dylib" on MacOS
    set(USD_LIB_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX}
        CACHE STRING "Extension of USD libraries")
endif ()

find_library(USD_LIBRARY
    NAMES
        ${USD_LIB_PREFIX}usd${USD_LIB_SUFFIX}
    HINTS
        ${PXR_USD_LOCATION}
        $ENV{PXR_USD_LOCATION}
    PATH_SUFFIXES
        lib
    DOC
        "Main USD library"
)

get_filename_component(USD_LIBRARY_DIR ${USD_LIBRARY} DIRECTORY)

# Get the boost version from the one built with USD
if(USD_INCLUDE_DIR)
    file(GLOB _USD_VERSION_HPP_FILE "${USD_INCLUDE_DIR}/boost-*/boost/version.hpp")
    list(LENGTH _USD_VERSION_HPP_FILE found_one)
    if(${found_one} STREQUAL "1")
        list(GET _USD_VERSION_HPP_FILE 0 USD_VERSION_HPP)
        file(STRINGS
            "${USD_VERSION_HPP}"
            _usd_tmp
            REGEX "#define BOOST_VERSION .*$")
        string(REGEX MATCH "[0-9]+" USD_BOOST_VERSION ${_usd_tmp})
        unset(_usd_tmp)
        unset(_USD_VERSION_HPP_FILE)
        unset(USD_VERSION_HPP)
    endif()
endif()

message(STATUS "USD include dir: ${USD_INCLUDE_DIR}")
message(STATUS "USD library dir: ${USD_LIBRARY_DIR}")
message(STATUS "USD version: ${USD_VERSION}")
if(DEFINED USD_BOOST_VERSION)
    message(STATUS "USD Boost::boost version: ${USD_BOOST_VERSION}")
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(USD
    REQUIRED_VARS
        PXR_USD_LOCATION
        USD_INCLUDE_DIR
        USD_LIBRARY_DIR
        USD_GENSCHEMA
        USD_CONFIG_FILE
        USD_VERSION
        PXR_VERSION
    VERSION_VAR
        USD_VERSION
)
