#
# Simple module to find AdskUsdEditForward.
#
# This module searches for a valid AdskUsdEditForward installation.
# It searches for AdskUsdEditForward's libraries and include header files.
#


find_path(ADSKUSDEDITFORWARD_INCLUDE_DIR
    NAMES
        AdskUsdEditForward/UsdEditForwardApi.h
    HINTS
        ${ADSKUSDEDITFORWARD_ROOT_DIR}
    PATH_SUFFIXES
        include/
    DOC
        "UsdEditForward header path"
)

find_library(ADSKUSDEDITFORWARD_LIBRARY
    NAMES
        AdskUsdEditForward
    HINTS
        ${ADSKUSDEDITFORWARD_ROOT_DIR}
    PATH_SUFFIXES
        lib/
    DOC
        "UsdEditForward library path"
)

# Handle the QUIETLY and REQUIRED arguments and set AdskUsdEditForward_FOUND to TRUE if
# all listed variables are TRUE.

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AdskUsdEditForward
    REQUIRED_VARS
        ADSKUSDEDITFORWARD_INCLUDE_DIR
        ADSKUSDEDITFORWARD_LIBRARY
)
if (AdskUsdEditForward_FOUND)
    # This will follow a message "-- Found USD: <path> ..."
    message(STATUS "   UsdEditForward include dir is ${ADSKUSDEDITFORWARD_INCLUDE_DIR}")
    message(STATUS "   UsdEditForward library is ${ADSKUSDEDITFORWARD_LIBRARY}")
endif()
