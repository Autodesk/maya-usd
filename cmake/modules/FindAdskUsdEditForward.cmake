#
# Module to find AdskUsdEditForward.
#
# This module searches for a valid USD Edit Forward installation.
# See the find_package at the bottom for the list of variables that will be set.
#

message(STATUS "Finding Autodesk USD Edit Forward")

############################################################################
#
# C++ headers

if(DEFINED ADSKUSDEDITFORWARD_ROOT_DIR)
    message(DEPRECATION "ADSKUSDEDITFORWARD_ROOT_DIR is deprecated, please use ADSK_USD_EDIT_FORWARD_ROOT_DIR instead.")
    set(ADSK_USD_EDIT_FORWARD_ROOT_DIR ${ADSKUSDEDITFORWARD_ROOT_DIR})
endif()
if(DEFINED ENV{ADSKUSDEDITFORWARD_ROOT_DIR})
    message(DEPRECATION "Environment variable ADSKUSDEDITFORWARD_ROOT_DIR is deprecated, please use ADSK_USD_EDIT_FORWARD_ROOT_DIR instead.")
    set(ADSK_USD_EDIT_FORWARD_ROOT_DIR $ENV{ADSKUSDEDITFORWARD_ROOT_DIR})
endif()

find_path(ADSK_USD_EDIT_FORWARD_INCLUDE_DIR
    NAMES
        AdskUsdEditForward/UsdEditForwardApi.h
    HINTS
        $ENV{ADSK_USD_EDIT_FORWARD_ROOT_DIR}
        ${ADSK_USD_EDIT_FORWARD_ROOT_DIR}
    PATH_SUFFIXES
        include
    DOC
        "Edit Forward header path"
)

############################################################################
#
# Link libraries

find_library(ADSK_USD_EDIT_FORWARD_LIBRARY
    NAMES
        AdskUsdEditForward
    HINTS
        $ENV{ADSK_USD_EDIT_FORWARD_ROOT_DIR}
        ${ADSK_USD_EDIT_FORWARD_ROOT_DIR}
    PATH_SUFFIXES
        lib
    DOC
        "Edit Forward library path"
)

############################################################################
#
# Edit Forward package
#
# Handle the QUIETLY and REQUIRED arguments and set AdskUsdEditForward_FOUND
# to TRUE if all listed variables are TRUE.

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AdskUsdEditForward
    REQUIRED_VARS
        ADSK_USD_EDIT_FORWARD_INCLUDE_DIR
        ADSK_USD_EDIT_FORWARD_LIBRARY
)

# Report to the user where the package was found.

if (AdskUsdEditForward_FOUND)
    # This will follow a message "-- Found AdskUsdEditForward: <path> ..."
    message(STATUS "   Include dir: ${ADSK_USD_EDIT_FORWARD_INCLUDE_DIR}")
    message(STATUS "   Library: ${ADSK_USD_EDIT_FORWARD_LIBRARY}")
endif()
