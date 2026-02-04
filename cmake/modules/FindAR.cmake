#
# Simple module to find AdskAssetResolver.
#
# This module searches for a valid AdskAssetResolver installation.
# It searches for AdskAssetResolver's libraries and include header files.
#


find_path(AR_INCLUDE_DIR
    NAMES
        AdskAssetResolver/AdskAssetResolver.h
    HINTS
        $ENV{AR_ROOT_DIR}
        ${AR_ROOT_DIR}
    PATH_SUFFIXES
        include/
    DOC
        "AR header path"
)

find_library(AR_LIBRARY
    NAMES
        AdskAssetResolver
    HINTS
        $ENV{AR_ROOT_DIR}
        ${AR_ROOT_DIR}
    PATH_SUFFIXES
        lib/
    DOC
        "AR library path"
)

# Handle the QUIETLY and REQUIRED arguments and set AR_FOUND to TRUE if
# all listed variables are TRUE.

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AR
    REQUIRED_VARS
        AR_INCLUDE_DIR
        AR_LIBRARY
)
if (AR_FOUND)
    # This will follow a message "-- Found USD: <path> ..."
    message(STATUS "   AR include dir is ${AR_INCLUDE_DIR}")
    message(STATUS "   AR library is ${AR_LIBRARY}")
endif()
