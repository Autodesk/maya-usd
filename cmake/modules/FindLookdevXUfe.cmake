#
# Simple module to find LookdevXUfe.
#
# This module searches for a valid LookdevXUfe installation in the Maya devkit.
# It searches for LookdevXUfe's libraries and include files.
#
# Variables that will be defined:
# LookdevXUfe_FOUND            Defined if a LookdevXUfe installation has been detected
# LookdevXUfe_LIBRARY          Path to LookdevXUfe library
# LookdevXUfe_INCLUDE_DIR      Path to the LookdevXUfe include directory
# LookdevXUfe_VERSION          LookdevXUfe version (major.minor.patch) from LookdevXUfe.h
#

find_path(LookdevXUfe_INCLUDE_DIR
        LookdevXUfe/LookdevXUfe.h
    HINTS
        $ENV{LOOKDEVXUFE_INCLUDE_ROOT}
        ${LOOKDEVXUFE_INCLUDE_ROOT}
        ${MAYA_DEVKIT_LOCATION}
        $ENV{MAYA_DEVKIT_LOCATION}
        ${MAYA_LOCATION}
        $ENV{MAYA_LOCATION}
        ${MAYA_BASE_DIR}
    PATH_SUFFIXES
        devkit/ufe/extensions/lookdevXUfe/include
        include/
    DOC
        "LookdevXUfe header path"
)

# Get the LookdevXUfe_VERSION from LookdevXUfe.h
if(LookdevXUfe_INCLUDE_DIR AND EXISTS "${LookdevXUfe_INCLUDE_DIR}/LookdevXUfe/LookdevXUfe.h")
    # Parse the file and get the three lines that have the version info.
    file(STRINGS
        "${LookdevXUfe_INCLUDE_DIR}/LookdevXUfe/LookdevXUfe.h"
        _ldx_vers
        REGEX "#define[ ]+(LOOKDEVXUFE_MAJOR_VERSION|LOOKDEVXUFE_MINOR_VERSION|LOOKDEVXUFE_PATCH_LEVEL)[ ]+[0-9]+$")

    # Then extract the number from each one.
    foreach(_ldxufe_tmp ${_ldx_vers})
        if(_ldxufe_tmp MATCHES "#define[ ]+(LOOKDEVXUFE_MAJOR_VERSION|LOOKDEVXUFE_MINOR_VERSION|LOOKDEVXUFE_PATCH_LEVEL)[ ]+([0-9]+)$")
            set(${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
        endif()
    endforeach()
    set(LookdevXUfe_VERSION ${LOOKDEVXUFE_MAJOR_VERSION}.${LOOKDEVXUFE_MINOR_VERSION}.${LOOKDEVXUFE_PATCH_LEVEL})
endif()

find_library(LookdevXUfe_LIBRARY
    NAMES
        lookdevXUfe_${LOOKDEVXUFE_MAJOR_VERSION}_${LOOKDEVXUFE_MINOR_VERSION}
    HINTS
        $ENV{LOOKDEVXUFE_LIB_ROOT}
        ${LOOKDEVXUFE_LIB_ROOT}
        ${MAYA_DEVKIT_LOCATION}
        $ENV{MAYA_DEVKIT_LOCATION}
        ${MAYA_LOCATION}
        $ENV{MAYA_LOCATION}
        ${MAYA_BASE_DIR}
    PATHS
        ${UFE_LIBRARY_DIR}
    PATH_SUFFIXES
        devkit/ufe/extensions/lookdevXUfe/lib
        lib/
    DOC
        "LookdevXUfe library"
    NO_DEFAULT_PATH
)

# Handle the QUIETLY and REQUIRED arguments and set LookdevXUfe_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LookdevXUfe
    REQUIRED_VARS
        LookdevXUfe_INCLUDE_DIR
        LookdevXUfe_LIBRARY
    VERSION_VAR
        LookdevXUfe_VERSION
)

set(LOOKDEVXUFE_HAS_PYTHON_BINDINGS FALSE CACHE INTERNAL "PyLookdevXUfe")
if(LookdevXUfe_VERSION VERSION_GREATER_EQUAL "1.0.1" OR
    (LookdevXUfe_VERSION VERSION_LESS "1.0.0" AND LookdevXUfe_VERSION VERSION_GREATER_EQUAL "0.2.0"))
    set(LOOKDEVXUFE_HAS_PYTHON_BINDINGS TRUE CACHE INTERNAL "PyLookdevXUfe")
    message(STATUS "Maya has LookdevXUfe Python bindings")
endif()

set(LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION FALSE CACHE INTERNAL "LegacyMxLookdevXUfe")
if(LookdevXUfe_VERSION VERSION_GREATER_EQUAL "2.0.0" OR
    (LookdevXUfe_VERSION VERSION_LESS "1.0.0" AND LookdevXUfe_VERSION VERSION_GREATER_EQUAL "0.2.12"))
    set(LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION TRUE CACHE INTERNAL "LegacyMxLookdevXUfe")
    message(STATUS "Maya has LookdevXUfe Legacy MaterialX handling")
endif()
