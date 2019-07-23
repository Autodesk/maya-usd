# - MayaUSD finder module
# This module searches for a valid MayaUsd installation.
# It searches for MayaUSD's libraries and include header files
#
# Variables that will be defined:
# MAYAUSD_FOUND           Defined if a MAYAUSD installation has been detected
# MAYAUSD_LIBRARY         Path to MAYAUSD library
# MAYAUSD_INCLUDE_DIR     Path to the MAYAUSD's include directories
#

if(APPLE)
    find_path(MAYAUSD_LIBRARY_DIR 
        libmayaUsd.dylib
        HINTS
            "${MAYAUSD_LIB_ROOT}"
            "$ENV{MAYAUSD_LIB_ROOT}"
            "${MAYA_DEVKIT_LOCATION}"
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
            devkit/mayaUsd/lib
            lib/
        DOC
            "MayaUSD's libraries path"
    )
elseif(UNIX)
    find_path(MAYAUSD_LIBRARY_DIR
            libmayaUsd.so
         HINTS
            "${MAYAUSD_LIB_ROOT}"
            "$ENV{MAYAUSD_LIB_ROOT}"
            "${MAYA_DEVKIT_LOCATION}"
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
            devkit/mayaUsd/lib
            lib/
        DOC
            "MayaUSD's libraries path"
    )
elseif(WIN32)
    find_path(MAYAUSD_LIBRARY_DIR
            mayaUsd.lib
         HINTS
            "${MAYAUSD_LIB_ROOT}"
            "$ENV{MAYAUSD_LIB_ROOT}"
            "${MAYA_DEVKIT_LOCATION}"
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
            devkit/mayaUsd/lib
            lib/
        DOC
            "MayaUSD's libraries path"
    )
endif()

find_path(MAYAUSD_INCLUDE_DIR
        mayaUsd/mayaUsd.h
    HINTS
        "${MAYAUSD_INCLUDE_ROOT}"
        "$ENV{MAYAUSD_INCLUDE_ROOT}"
        "${MAYA_DEVKIT_LOCATION}"
        "${MAYA_LOCATION}"
        "$ENV{MAYA_LOCATION}"
        "${MAYA_BASE_DIR}"
    PATH_SUFFIXES
        devkit/mayaUsd/include
        include/
    DOC
        "MayaUSD's headers path"
)

# Use find_library to account for platform-specific library name prefixes
# (e.g. lib) and suffixes (e.g. .lib, .so, .dylib).
foreach(MAYAUSD_LIB mayaUsd)

    find_library(MAYAUSD_LIBRARY
        NAMES
            ${MAYAUSD_LIB}
        PATHS
            ${MAYAUSD_LIBRARY_DIR}
        NO_DEFAULT_PATH
    )

    if (MAYAUSD_LIBRARY)
        list(APPEND MAYAUSD_LIBRARIES ${MAYAUSD_LIBRARY})
    endif()

endforeach(MAYAUSD_LIB)

# If we weren't passed in the MayaUsd version info, read it from the header file.
if (NOT DEFINED MAYAUSD_MAJOR_VERSION)
    file(READ ${MAYAUSD_INCLUDE_DIR}/mayaUsd/mayaUsd.h MAYAUSD_MAIN_HEADER)
    string(REGEX REPLACE ".*\#define MAYAUSD_MAJOR_VERSION[ ]+([0-9]+).*" "\\1" MAYAUSD_MAJOR_VERSION ${MAYAUSD_MAIN_HEADER})
    string(REGEX REPLACE ".*\#define MAYAUSD_MINOR_VERSION[ ]+([0-9]+).*" "\\1" MAYAUSD_MINOR_VERSION ${MAYAUSD_MAIN_HEADER})
    string(REGEX REPLACE ".*\#define MAYAUSD_PATCH_LEVEL[ ]+([0-9]+).*" "\\1" MAYAUSD_PATCH_LEVEL ${MAYAUSD_MAIN_HEADER})
endif()
set(MAYAUSD_VERSION "${MAYAUSD_MAJOR_VERSION}.${MAYAUSD_MINOR_VERSION}.${MAYAUSD_PATCH_LEVEL}")

# handle the QUIETLY and REQUIRED arguments and set MAYAUSD_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(MAYAUSD
    REQUIRED_VARS
        MAYAUSD_INCLUDE_DIR
        MAYAUSD_LIBRARIES
    VERSION_VAR
        MAYAUSD_VERSION
)
