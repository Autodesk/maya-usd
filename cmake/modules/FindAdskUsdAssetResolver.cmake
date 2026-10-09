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
        AdskUsdAssetResolver/Resolver.h
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
        AdskUsdAssetResolver
    HINTS
        $ENV{ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
        ${ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
    PATH_SUFFIXES
        lib
    DOC
        "Asset Resolver library path"
)
find_library(ADSK_USD_ASSET_RESOLVER_DIALOG_LIBRARY
    NAMES
        AdskUsdAssetResolverExtensions
    HINTS
        $ENV{ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
        ${ADSK_USD_ASSET_RESOLVER_ROOT_DIR}
    PATH_SUFFIXES
        lib
    DOC
        "Asset Resolver Dialog library path"
)

###########################################################################
#
# Asset Resolver version

if(ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR)
    file(
        STRINGS
        ${ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR}/AdskUsdAssetResolver/AdskUsdAssetResolverVersion.h
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
    message(STATUS "  Include dir: ${ADSK_USD_ASSET_RESOLVER_INCLUDE_DIR}")
    message(STATUS "  Libraries: ${ADSK_USD_ASSET_RESOLVER_LIBRARY} ${ADSK_USD_ASSET_RESOLVER_DIALOG_LIBRARY}")
endif()

