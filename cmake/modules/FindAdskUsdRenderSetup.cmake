#
# Module to find UsdRenderSetup.
#
# This module searches for a valid USD Render Setup installation.
# See the find_package at the bottom for the list of variables that will be set.
#

message(STATUS "Finding Autodesk USD Render Setup")

############################################################################
#
# C++ headers

find_path(ADSK_USD_RENDER_SETUP_INCLUDE_DIR
    NAMES
        AdskUsdRenderSetup/AdskUsdRenderSetupVersion.h
    HINTS
        $ENV{ADSK_USD_RENDER_SETUP_ROOT_DIR}
        ${ADSK_USD_RENDER_SETUP_ROOT_DIR}
    PATH_SUFFIXES
        include
    DOC
        "Render Setup header path"
)

############################################################################
#
# Link libraries

find_library(ADSK_USD_RENDER_SETUP_LIBRARY
    NAMES
        AdskUsdRenderSetup
    HINTS
        $ENV{ADSK_USD_RENDER_SETUP_ROOT_DIR}
        ${ADSK_USD_RENDER_SETUP_ROOT_DIR}
    PATH_SUFFIXES
        lib
    DOC
        "Render Setup library path"
)

###########################################################################
#
# Render Setup version

if(ADSK_USD_RENDER_SETUP_INCLUDE_DIR)
    set(adsk_rs_version_header "${ADSK_USD_RENDER_SETUP_INCLUDE_DIR}/AdskUsdRenderSetup/AdskUsdRenderSetupVersion.h")
    file(
        STRINGS
        ${adsk_rs_version_header}
        ADSK_USD_RENDER_SETUP_VERSION
        REGEX "define RENDER_SETUP_VERSION .*")
    if(ADSK_USD_RENDER_SETUP_VERSION)
        string(REGEX MATCHALL "[0-9.]+" ADSK_USD_RENDER_SETUP_VERSION ${ADSK_USD_RENDER_SETUP_VERSION})
    endif()
endif()

###########################################################################
#
# ABI level
#
# ADSK_ABI tells the render setup headers which release's API surface to expose.
# Part of that surface is not ABI-compatible across releases - StageRenderInventory
# gains members, and CollectInventory returns it by value - so a consumer that
# disagrees with the component about ADSK_ABI corrupts the stack rather than
# failing to build. The component is published per Maya release, so the matching
# value is the Maya year we are building against.

if(ADSK_USD_RENDER_SETUP_INCLUDE_DIR AND MAYA_APP_VERSION)
    set(ADSK_USD_RENDER_SETUP_ABI ${MAYA_APP_VERSION})
endif()

############################################################################
#
# Render Setup package
#
# Handle the QUIETLY and REQUIRED arguments and set AdskUsdRenderSetup_FOUND
# to TRUE if all listed variables are TRUE.

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AdskUsdRenderSetup
    REQUIRED_VARS
        ADSK_USD_RENDER_SETUP_INCLUDE_DIR
        ADSK_USD_RENDER_SETUP_LIBRARY
    VERSION_VAR
        ADSK_USD_RENDER_SETUP_VERSION
)

# Report to the user where the package was found.

if (AdskUsdRenderSetup_FOUND)
    # This will follow a message "-- Found AdskUsdRenderSetup: <path> ..."
    message(STATUS "  Version: ${ADSK_USD_RENDER_SETUP_VERSION}")
    message(STATUS "  Include dir: ${ADSK_USD_RENDER_SETUP_INCLUDE_DIR}")
    message(STATUS "  Libraries: ${ADSK_USD_RENDER_SETUP_LIBRARY}")
    if(ADSK_USD_RENDER_SETUP_ABI)
        message(STATUS "  ABI: ${ADSK_USD_RENDER_SETUP_ABI}")
    else()
        message(WARNING "AdskUsdRenderSetup: ADSK_ABI unset (no MAYA_APP_VERSION); "
                        "headers may disagree with the component about StageRenderInventory.")
    endif()
endif()
