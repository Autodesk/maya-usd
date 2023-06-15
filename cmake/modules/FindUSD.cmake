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

# This component is optional.
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

if(NOT DEFINED PXR_VERSION)
    message(FATAL_ERROR "Expected PXR_VERSION defined in pxrConfig.cmake")
endif()
# Starting in core USD 21.05, pxrConfig.cmake provides the various USD
# version numbers as CMake variables, in which case PXR_VERSION should have
# been defined, along with the major, minor, and patch version numbers.
# The only thing we need to do is assemble the USD_VERSION version string.
set(USD_VERSION ${PXR_MAJOR_VERSION}.${PXR_MINOR_VERSION}.${PXR_PATCH_VERSION})

# Set special Autodesk USD version. We use this to communicate whether or not
# there are extra patches on-top of the normal USD build.
set(ADSK_USD_VERSION ${USD_VERSION})
if (USD_VERSION VERSION_LESS "0.23.02")
    # In the security fix there was a new inline method "max_size" added so we
    # can use that to determine if this USD build has the security fix.
    if (USD_INCLUDE_DIR AND EXISTS "${USD_INCLUDE_DIR}/pxr/base/vt/array.h")
        file(STRINGS ${USD_INCLUDE_DIR}/pxr/base/vt/array.h USD_HAS_SECURITY_PATCH REGEX "constexpr size_t max_size\(\)")
        if (USD_HAS_SECURITY_PATCH)
            set(ADSK_USD_VERSION "${ADSK_USD_VERSION}-ad1")
            message(STATUS "USD v${USD_VERSION} has security patches.")
        endif()
    endif()
endif()

# USD_LIB_PREFIX should match the PXR_LIB_PREFIX used
# for building USD (and shouldn't need to be touched if PXR_LIB_PREFIX was not
# used / left at it's default value). Starting with USD 21.11, the default
# value for PXR_LIB_PREFIX was changed to include "usd_".

set(USD_LIB_PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}usd_"
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

# See if MaterialX shaders with color4 inputs exist natively in Sdr:
# Not yet in a tagged USD version: https://github.com/PixarAnimationStudios/USD/pull/1894
set(USD_HAS_COLOR4_SDR_SUPPORT FALSE CACHE INTERNAL "USD.Sdr.PropertyTypes.Color4")
if (USD_INCLUDE_DIR AND EXISTS "${USD_INCLUDE_DIR}/pxr/usd/sdr/shaderProperty.h")
    file(STRINGS ${USD_INCLUDE_DIR}/pxr/usd/sdr/shaderProperty.h USD_HAS_API REGEX "Color4")
    if(USD_HAS_API)
        set(USD_HAS_COLOR4_SDR_SUPPORT TRUE CACHE INTERNAL "USD.Sdr.PropertyTypes.Color4")
        message(STATUS "USD has new Sdr.PropertyTypes.Color4")
    endif()
endif()

# See if MaterialX shaders have full Metadata imported:
# Not yet in a tagged USD version: https://github.com/PixarAnimationStudios/USD/pull/1895
set(USD_HAS_MX_METADATA_SUPPORT FALSE CACHE INTERNAL "USD.MaterialX.Metadata")
if (USD_LIBRARY_DIR AND EXISTS "${USD_LIBRARY_DIR}/${USD_LIB_PREFIX}usdMtlx${CMAKE_SHARED_LIBRARY_SUFFIX}")
    file(STRINGS ${USD_LIBRARY_DIR}/${USD_LIB_PREFIX}usdMtlx${CMAKE_SHARED_LIBRARY_SUFFIX} USD_HAS_API REGEX "uisoftmin")
    if(USD_HAS_API)
        set(USD_HAS_MX_METADATA_SUPPORT TRUE CACHE INTERNAL "USD.MaterialX.Metadata")
        message(STATUS "USD has MaterialX metadata support")
    endif()
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(USD
    REQUIRED_VARS
        PXR_USD_LOCATION
        USD_INCLUDE_DIR
        USD_LIBRARY_DIR
        USD_CONFIG_FILE
        USD_VERSION
        ADSK_USD_VERSION
        PXR_VERSION
    VERSION_VAR
        USD_VERSION
)

if (USD_FOUND)
    # This will follow a message "-- Found USD: <path> ..."
    message(STATUS "   USD include dir: ${USD_INCLUDE_DIR}")
    message(STATUS "   USD library dir: ${USD_LIBRARY_DIR}")
    if (USD_GENSCHEMA)
        message(STATUS "   usdGenSchema: ${USD_GENSCHEMA}")
    endif()
    message(STATUS "   USD version: ${USD_VERSION}")
    message(STATUS "   Autodesk USD version: ${ADSK_USD_VERSION}")
    if(DEFINED USD_BOOST_VERSION)
        message(STATUS "   USD Boost::boost version: ${USD_BOOST_VERSION}")
    endif()
endif()
