#
# Simple module to find UFE.
#
# This module searches for a valid UFE installation.
# It searches for UFE's libraries and include header files.
#
# Variables that will be defined:
# UFE_FOUND           Defined if a UFE installation has been detected
# UFE_LIBRARY         Path to UFE library
# UFE_INCLUDE_DIR     Path to the UFE include directory
#

find_path(UFE_INCLUDE_DIR
        ufe/versionInfo.h
    HINTS
        ${UFE_INCLUDE_ROOT}
        ${MAYA_DEVKIT_LOCATION}
        ${MAYA_LOCATION}
        $ENV{MAYA_LOCATION}
        ${MAYA_BASE_DIR}
    PATH_SUFFIXES
        devkit/ufe/include
        include/
    DOC
        "UFE header path"
)

# get UFE_VERSION from ufe.h
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/ufe.h")
    foreach(_ufe_comp MAJOR MINOR)
        file(STRINGS
            "${UFE_INCLUDE_DIR}/ufe/ufe.h"
            _ufe_tmp
            REGEX "#define UFE_${_ufe_comp}_VERSION .*$")
        string(REGEX MATCHALL "[0-9]+" UFE_${_ufe_comp}_VERSION ${_ufe_tmp})
    endforeach()
    foreach(_ufe_comp PATCH)
        file(STRINGS
            "${UFE_INCLUDE_DIR}/ufe/ufe.h"
            _ufe_tmp
            REGEX "#define UFE_${_ufe_comp}_LEVEL .*$")
        string(REGEX MATCHALL "[0-9]+" UFE_${_ufe_comp}_LEVEL ${_ufe_tmp})
    endforeach()

    set(UFE_VERSION ${UFE_MAJOR_VERSION}.${UFE_MINOR_VERSION}.${UFE_PATCH_LEVEL})
endif()

find_library(UFE_LIBRARY
    NAMES
        ufe_${UFE_MAJOR_VERSION}
    HINTS
        ${UFE_LIB_ROOT}
        ${MAYA_DEVKIT_LOCATION}
        ${MAYA_LOCATION}
        $ENV{MAYA_LOCATION}
        ${MAYA_BASE_DIR}
    PATHS
        ${UFE_LIBRARY_DIR}
    PATH_SUFFIXES
        devkit/ufe/lib
        lib/
    DOC
        "UFE library"
    NO_DEFAULT_PATH
)

# Handle the QUIETLY and REQUIRED arguments and set UFE_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(UFE
    REQUIRED_VARS
        UFE_INCLUDE_DIR
        UFE_LIBRARY
    VERSION_VAR
        UFE_VERSION
)
