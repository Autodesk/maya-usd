#
# Module to find AdskUsdAssetResolver.
#
# This module searches for a valid USD Asset Resolver installation.
# See the find_package at the bottom for the list of variables that will be set.
#

message(STATUS "Finding Autodesk USD Asset Resolver")

if(DEFINED AR_ROOT_DIR)
    message(DEPRECATION "AR_ROOT_DIR is deprecated, please use ADSK_USD_ASSET_RESOLVER_ROOT_DIR instead.")
    set(ADSK_USD_ASSET_RESOLVER_ROOT_DIR ${AR_ROOT_DIR})
endif()
if(DEFINED ENV{AR_ROOT_DIR})
    message(DEPRECATION "Environment variable AR_ROOT_DIR is deprecated, please use ADSK_USD_ASSET_RESOLVER_ROOT_DIR instead.")
    set(ADSK_USD_ASSET_RESOLVER_ROOT_DIR $ENV{AR_ROOT_DIR})
endif()

set(ADSK_USD_ASSET_RESOLVER_LAYOUT "AUTO" CACHE STRING "Asset Resolver layout to use: AUTO, PREOSS, or OSS")
set_property(CACHE ADSK_USD_ASSET_RESOLVER_LAYOUT PROPERTY STRINGS AUTO PREOSS OSS)

############################################################################
#
# C++ headers

find_path(ADSK_USD_ASSET_RESOLVER_PREOSS_INCLUDE_DIR
    NAMES
        AdskAssetResolver/AdskAssetResolver.h
    HINTS
        $ENV{ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
        ${ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
    PATH_SUFFIXES
        include
    DOC
        "Pre-open-source Asset Resolver header path"
)
find_path(ADSK_USD_ASSET_RESOLVER_OSS_INCLUDE_DIR
    NAMES
        AdskUsdAssetResolver/Resolver.h
    HINTS
        $ENV{ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
        ${ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
    PATH_SUFFIXES
        include
    DOC
        "Open-source Asset Resolver header path"
)

############################################################################
#
# Link libraries

find_library(ADSK_USD_ASSET_RESOLVER_PREOSS_LIBRARY
    NAMES
        AdskAssetResolver
    HINTS
        $ENV{ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
        ${ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
    PATH_SUFFIXES
        lib
    DOC
        "Pre-open-source Asset Resolver library path"
)
find_library(ADSK_USD_ASSET_RESOLVER_PREOSS_DIALOG_LIBRARY
    NAMES
        AssetResolverExtensions
    HINTS
        $ENV{ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
        ${ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
    PATH_SUFFIXES
        lib
    DOC
        "Pre-open-source Asset Resolver Dialog library path"
)
find_library(ADSK_USD_ASSET_RESOLVER_OSS_LIBRARY
    NAMES
        AdskUsdAssetResolver
    HINTS
        $ENV{ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
        ${ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
    PATH_SUFFIXES
        lib
    DOC
        "Open-source Asset Resolver library path"
)
find_library(ADSK_USD_ASSET_RESOLVER_OSS_DIALOG_LIBRARY
    NAMES
        AdskUsdAssetResolverExtensions
    HINTS
        $ENV{ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
        ${ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
    PATH_SUFFIXES
        lib
    DOC
        "Open-source Asset Resolver Dialog library path"
)

set(_ADSK_USD_ASSET_RESOLVER_PREOSS_FOUND FALSE)
if(ADSK_USD_ASSET_RESOLVER_PREOSS_INCLUDE_DIR AND ADSK_USD_ASSET_RESOLVER_PREOSS_LIBRARY AND ADSK_USD_ASSET_RESOLVER_PREOSS_DIALOG_LIBRARY)
    set(_ADSK_USD_ASSET_RESOLVER_PREOSS_FOUND TRUE)
endif()

set(_ADSK_USD_ASSET_RESOLVER_OSS_FOUND FALSE)
if(ADSK_USD_ASSET_RESOLVER_OSS_INCLUDE_DIR AND ADSK_USD_ASSET_RESOLVER_OSS_LIBRARY AND ADSK_USD_ASSET_RESOLVER_OSS_DIALOG_LIBRARY)
    set(_ADSK_USD_ASSET_RESOLVER_OSS_FOUND TRUE)
endif()

set(ADSK_USD_ASSET_RESOLVER_RESOLVED_LAYOUT "" CACHE INTERNAL "Resolved Asset Resolver layout")
set(ADSK_USD_ASSET_RESOLVER_LAYOUT_PREOSS FALSE CACHE INTERNAL "Asset Resolver uses the pre-open-source layout")
set(ADSK_USD_ASSET_RESOLVER_LAYOUT_OSS FALSE CACHE INTERNAL "Asset Resolver uses the open-source layout")
unset(ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR CACHE)
unset(ADSK_USD_ASSET_RESOLVER_LIBRARY CACHE)
unset(ADSK_USD_ASSET_RESOLVER_DIALOG_LIBRARY CACHE)

if(ADSK_USD_ASSET_RESOLVER_LAYOUT STREQUAL "PREOSS")
    set(ADSK_USD_ASSET_RESOLVER_RESOLVED_LAYOUT PREOSS CACHE INTERNAL "Resolved Asset Resolver layout")
elseif(ADSK_USD_ASSET_RESOLVER_LAYOUT STREQUAL "OSS")
    set(ADSK_USD_ASSET_RESOLVER_RESOLVED_LAYOUT OSS CACHE INTERNAL "Resolved Asset Resolver layout")
elseif(_ADSK_USD_ASSET_RESOLVER_OSS_FOUND)
    set(ADSK_USD_ASSET_RESOLVER_RESOLVED_LAYOUT OSS CACHE INTERNAL "Resolved Asset Resolver layout")
elseif(_ADSK_USD_ASSET_RESOLVER_PREOSS_FOUND)
    set(ADSK_USD_ASSET_RESOLVER_RESOLVED_LAYOUT PREOSS CACHE INTERNAL "Resolved Asset Resolver layout")
endif()

if(ADSK_USD_ASSET_RESOLVER_RESOLVED_LAYOUT STREQUAL "OSS")
    set(ADSK_USD_ASSET_RESOLVER_LAYOUT_OSS TRUE CACHE INTERNAL "Asset Resolver uses the open-source layout")
    set(ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR ${ADSK_USD_ASSET_RESOLVER_OSS_INCLUDE_DIR} CACHE PATH "Asset Resolver header path")
    set(ADSK_USD_ASSET_RESOLVER_LIBRARY ${ADSK_USD_ASSET_RESOLVER_OSS_LIBRARY} CACHE FILEPATH "Asset Resolver library path")
    set(ADSK_USD_ASSET_RESOLVER_DIALOG_LIBRARY ${ADSK_USD_ASSET_RESOLVER_OSS_DIALOG_LIBRARY} CACHE FILEPATH "Asset Resolver Dialog library path")
    set(ADSK_USD_ASSET_RESOLVER_INCLUDE_SUBDIR AdskUsdAssetResolver CACHE INTERNAL "Asset Resolver include subdirectory")
    set(ADSK_USD_ASSET_RESOLVER_VERSION_HEADER AdskUsdAssetResolverVersion.h CACHE INTERNAL "Asset Resolver version header")
elseif(ADSK_USD_ASSET_RESOLVER_RESOLVED_LAYOUT STREQUAL "PREOSS")
    set(ADSK_USD_ASSET_RESOLVER_LAYOUT_PREOSS TRUE CACHE INTERNAL "Asset Resolver uses the pre-open-source layout")
    set(ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR ${ADSK_USD_ASSET_RESOLVER_PREOSS_INCLUDE_DIR} CACHE PATH "Asset Resolver header path")
    set(ADSK_USD_ASSET_RESOLVER_LIBRARY ${ADSK_USD_ASSET_RESOLVER_PREOSS_LIBRARY} CACHE FILEPATH "Asset Resolver library path")
    set(ADSK_USD_ASSET_RESOLVER_DIALOG_LIBRARY ${ADSK_USD_ASSET_RESOLVER_PREOSS_DIALOG_LIBRARY} CACHE FILEPATH "Asset Resolver Dialog library path")
    set(ADSK_USD_ASSET_RESOLVER_INCLUDE_SUBDIR AdskAssetResolver CACHE INTERNAL "Asset Resolver include subdirectory")
    set(ADSK_USD_ASSET_RESOLVER_VERSION_HEADER AdskAssetResolverVersion.h CACHE INTERNAL "Asset Resolver version header")
endif()

###########################################################################
#
# Asset Resolver version

if(ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR AND ADSK_USD_ASSET_RESOLVER_INCLUDE_SUBDIR AND ADSK_USD_ASSET_RESOLVER_VERSION_HEADER)
    file(
        STRINGS
        ${ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR}/${ADSK_USD_ASSET_RESOLVER_INCLUDE_SUBDIR}/${ADSK_USD_ASSET_RESOLVER_VERSION_HEADER}
        ADSK_USD_ASSET_RESOLVER_VERSION
        REGEX "#define ADSK_USD_ASSET_RESOLVER_VERSION \\\"[0-9.]+\\\"")
    if(ADSK_USD_ASSET_RESOLVER_VERSION)
        string(REGEX MATCHALL "[0-9.]+" ADSK_USD_ASSET_RESOLVER_VERSION ${ADSK_USD_ASSET_RESOLVER_VERSION})
    endif()
endif()

############################################################################
#
# Asset Resolver package
#
# Handle the QUIETLY and REQUIRED arguments and set AdskUsdAssetResolver_FOUND
# to TRUE if all listed variables are TRUE.

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AdskUsdAssetResolver
    REQUIRED_VARS
        ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR
        ADSK_USD_ASSET_RESOLVER_LIBRARY
        ADSK_USD_ASSET_RESOLVER_DIALOG_LIBRARY
    VERSION_VAR
        ADSK_USD_ASSET_RESOLVER_VERSION
)

# Report to the user where the package was found.

if (AdskUsdAssetResolver_FOUND)
    # This will follow a message "-- Found AdskUsdAssetResolver: <path> ..."
    message(STATUS "  Version: ${ADSK_USD_ASSET_RESOLVER_VERSION}")
    message(STATUS "  Layout: ${ADSK_USD_ASSET_RESOLVER_RESOLVED_LAYOUT}")
    message(STATUS "  Include dir: ${ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR}")
    message(STATUS "  Libraries: ${ADSK_USD_ASSET_RESOLVER_LIBRARY} ${ADSK_USD_ASSET_RESOLVER_DIALOG_LIBRARY}")
endif()

set(ADSK_USD_ASSET_RESOLVER_CONTEXTDATA_HAS_PATHARRAY FALSE CACHE INTERNAL "arPathArray")
if(ADSK_USD_ASSET_RESOLVER_RESOLVED_LAYOUT STREQUAL "OSS")
    set(ADSK_USD_ASSET_RESOLVER_CONTEXTDATA_HAS_PATHARRAY TRUE CACHE INTERNAL "arAssetResolverContextDataHasPatharray")
    message(STATUS "  Asset Resolver has PathArray support")
else()
    set(_ADSK_USD_ASSET_RESOLVER_CONTEXT_DATA_HEADER "${ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR}/${ADSK_USD_ASSET_RESOLVER_INCLUDE_SUBDIR}/AssetResolverContextData.h")
    if(ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR AND EXISTS "${_ADSK_USD_ASSET_RESOLVER_CONTEXT_DATA_HEADER}")
        file(STRINGS ${_ADSK_USD_ASSET_RESOLVER_CONTEXT_DATA_HEADER} AR_HAS_API REGEX "PathArray")
        if(AR_HAS_API)
            set(ADSK_USD_ASSET_RESOLVER_CONTEXTDATA_HAS_PATHARRAY TRUE CACHE INTERNAL "arAssetResolverContextDataHasPatharray")
            message(STATUS "  Asset Resolver has PathArray support")
        endif()
    endif()
endif()