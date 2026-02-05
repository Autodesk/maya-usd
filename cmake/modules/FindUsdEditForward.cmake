#
# Simple module to find AdskUsdEditForward.
#
# This module searches for a valid AdskUsdEditForward installation.
# It searches for AdskUsdEditForward's libraries and include header files.
#


find_path(USDEDITFORWARD_INCLUDE_DIR
    NAMES
        AdskUsdEditForward/UsdEditForwardApi.h
    HINTS
        ${USDEDITFORWARD_ROOT_DIR}
    PATH_SUFFIXES
        include/
    DOC
        "UsdEditForward header path"
)

find_library(USDEDITFORWARD_LIBRARY
    NAMES
        AdskUsdEditForward
    HINTS
        ${USDEDITFORWARD_ROOT_DIR}
    PATH_SUFFIXES
        lib/
    DOC
        "UsdEditForward library path"
)

# Handle the QUIETLY and REQUIRED arguments and set UsdEditForward_FOUND to TRUE if
# all listed variables are TRUE.

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UsdEditForward
    REQUIRED_VARS
        USDEDITFORWARD_INCLUDE_DIR
        USDEDITFORWARD_LIBRARY
)
if (UsdEditForward_FOUND)
    # This will follow a message "-- Found USD: <path> ..."
    message(STATUS "   UsdEditForward include dir is ${USDEDITFORWARD_INCLUDE_DIR}")
    message(STATUS "   UsdEditForward library is ${USDEDITFORWARD_LIBRARY}")
endif()
