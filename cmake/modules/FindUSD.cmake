# Simple module to find USD.

# Note that on Windows with USD <= 0.19.11, USD_LIB_PREFIX should be left at
# default (or set to empty string), even if PXR_LIB_PREFIX was specified when
# building core USD, due to a bug.

# On all other platforms / versions, it should match the PXR_LIB_PREFIX used
# for building USD (and shouldn't need to be touched if PXR_LIB_PREFIX was not
# used / left at it's default value).

set(USD_LIB_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX}
    CACHE STRING "Prefix of USD libraries; generally matches the PXR_LIB_PREFIX used when building core USD")

if (WIN32)
    # ".lib" on Windows
    set(USD_LIB_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX}
        CACHE STRING "Extension of USD libraries")
else ()
    # ".so" on Linux, ".dylib" on MacOS
    set(USD_LIB_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX}
        CACHE STRING "Extension of USD libraries")
endif ()

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

# ensure PXR_USD_LOCATION is defined
if(NOT DEFINED PXR_USD_LOCATION)
    if(DEFINED ENV{PXR_USD_LOCATION})
        set(PXR_USD_LOCATION "$ENV{PXR_USD_LOCATION}")
    else()
        get_filename_component(PXR_USD_LOCATION "${USD_CONFIG_FILE}" DIRECTORY)
    endif()
endif()

# account for possibility that PXR_USD_LOCATION was passed in as a hint-list
list(LENGTH PXR_USD_LOCATION listlen)
if(listlen GREATER 1)
    get_filename_component(PXR_USD_LOCATION "${USD_CONFIG_FILE}" DIRECTORY)
endif()

if(USD_INCLUDE_DIR AND EXISTS "${USD_INCLUDE_DIR}/pxr/pxr.h")
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

message(STATUS "USD include dir: ${USD_INCLUDE_DIR}")
message(STATUS "USD library dir: ${USD_LIBRARY_DIR}")
message(STATUS "USD version: ${USD_VERSION}")

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
