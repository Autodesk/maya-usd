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

############################################################################
#
# C++ headers

find_path(ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR
    NAMES
        AdskAssetResolver/AdskAssetResolver.h
    HINTS
        $ENV{ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
        ${ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
    PATH_SUFFIXES
        include
    DOC
        "Asset Resolver header path"
)

############################################################################
#
# Link libraries

find_library(ADSK_USD_ASSET_RESOLVER_LIBRARY
    NAMES
        AdskAssetResolver
    HINTS
        $ENV{ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
        ${ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
    PATH_SUFFIXES
        lib
    DOC
        "Asset Resolver library path"
)
find_library(ADSK_USD_ASSET_RESOLVER_PREFERENCES_LIBRARY
    NAMES
        AssetResolverPreferences
    HINTS
        $ENV{ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
        ${ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
    PATH_SUFFIXES
        lib
    DOC
        "Asset Resolver Preferences library path"
)

###########################################################################
#
# Asset Resolver version

if(ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR)
    file(
        STRINGS
        ${ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR}/AdskAssetResolver/AdskAssetResolverVersion.h
        ADSK_USD_ASSET_RESOLVER_VERSION
        REGEX "define ADSK_USD_ASSET_RESOLVER_VERSION .*")
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
        ADSK_USD_ASSET_RESOLVER_PREFERENCES_LIBRARY
)

# Report to the user where the package was found.

if (AdskUsdAssetResolver_FOUND)
    # This will follow a message "-- Found AdskUsdAssetResolver: <path> ..."
    message(STATUS "  Version: ${ADSK_USD_ASSET_RESOLVER_VERSION}")
    message(STATUS "  Include dir: ${ADSK_USD_ASSE_RESOLVER_INCLUDE_DIR}")
    message(STATUS "  Libraries: ${ADSK_USD_ASSET_RESOLVER_LIBRARY} ${ADSK_USD_ASSET_RESOLVER_PREFERENCES_LIBRARY}")
endif()

set(ADSK_USD_ASSET_RESOLVER_CONTEXTDATA_HAS_PATHARRAY FALSE CACHE INTERNAL "arPathArray")
if(ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR AND EXISTS "${ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR}/AdskAssetResolver/AssetResolverContextData.h")
    file(STRINGS ${ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR}/AdskAssetResolver/AssetResolverContextData.h AR_HAS_API REGEX "PathArray")
    if(AR_HAS_API)
        set(ADSK_USD_ASSET_RESOLVER_CONTEXTDATA_HAS_PATHARRAY TRUE CACHE INTERNAL "arAssetResolverContextDataHasPatharray")
        message(STATUS "  AssetResolverContextData has PathArray")
    endif()
endif()