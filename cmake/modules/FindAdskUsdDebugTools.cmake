#
# Module to find AdskUsdDebugTools.
#
# This module searches for a valid Autodesk USD Debug Tools installation
# (libraries AdskUsdDebugCore and AdskUsdDebugUI).
# See the find_package at the bottom for the list of variables that will be set.
#

message(STATUS "Finding Autodesk USD Debug Tools")

############################################################################
#
# C++ headers

find_path(ADSK_USD_DEBUG_TOOLS_INCLUDE_DIR
    NAMES
        UsdDebugCore/AdskUsdDebugCoreExport.h
    HINTS
        $ENV{ADSK_USD_DEBUG_TOOLS_ROOT_DIR}
        ${ADSK_USD_DEBUG_TOOLS_ROOT_DIR}
    PATH_SUFFIXES
        include
    DOC
        "USD Debug Tools header path"
)

############################################################################
#
# Link libraries

find_library(ADSK_USD_DEBUG_TOOLS_CORE_LIBRARY
    NAMES
        AdskUsdDebugCore
    HINTS
        $ENV{ADSK_USD_DEBUG_TOOLS_ROOT_DIR}
        ${ADSK_USD_DEBUG_TOOLS_ROOT_DIR}
    PATH_SUFFIXES
        lib
    DOC
        "USD Debug Tools core library path"
)
find_library(ADSK_USD_DEBUG_TOOLS_UI_LIBRARY
    NAMES
        AdskUsdDebugUI
    HINTS
        $ENV{ADSK_USD_DEBUG_TOOLS_ROOT_DIR}
        ${ADSK_USD_DEBUG_TOOLS_ROOT_DIR}
    PATH_SUFFIXES
        lib
    DOC
        "USD Debug Tools UI library path"
)

###########################################################################
#
# USD Debug Tools version

set(ADSK_USD_DEBUG_TOOLS_VERSION "")
if(ADSK_USD_DEBUG_TOOLS_INCLUDE_DIR AND EXISTS "${ADSK_USD_DEBUG_TOOLS_INCLUDE_DIR}/UsdDebugCore/AdskUsdDebugCoreVersion.h")
    file(
        STRINGS
        ${ADSK_USD_DEBUG_TOOLS_INCLUDE_DIR}/UsdDebugCore/AdskUsdDebugCoreVersion.h
        ADSK_USD_DEBUG_TOOLS_VERSION
        REGEX "define ADSK_USD_DEBUG_TOOLS_VERSION .*")
    if(ADSK_USD_DEBUG_TOOLS_VERSION)
        string(REGEX MATCHALL "[0-9.]+" ADSK_USD_DEBUG_TOOLS_VERSION ${ADSK_USD_DEBUG_TOOLS_VERSION})
    endif()
endif()

############################################################################
#
# USD Debug Tools package
#
# Handle the QUIETLY and REQUIRED arguments and set AdskUsdDebugTools_FOUND
# to TRUE if all listed variables are TRUE.

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AdskUsdDebugTools
    REQUIRED_VARS
        ADSK_USD_DEBUG_TOOLS_INCLUDE_DIR
        ADSK_USD_DEBUG_TOOLS_CORE_LIBRARY
        ADSK_USD_DEBUG_TOOLS_UI_LIBRARY
    VERSION_VAR
        ADSK_USD_DEBUG_TOOLS_VERSION
)

# Report to the user where the package was found.

if (AdskUsdDebugTools_FOUND)
    # This will follow a message "-- Found AdskUsdDebugTools: <path> ..."
    message(STATUS "  Version: ${ADSK_USD_DEBUG_TOOLS_VERSION}")
    message(STATUS "  Include dir: ${ADSK_USD_DEBUG_TOOLS_INCLUDE_DIR}")
    message(STATUS "  Libraries: ${ADSK_USD_DEBUG_TOOLS_CORE_LIBRARY} ${ADSK_USD_DEBUG_TOOLS_UI_LIBRARY}")
endif()
