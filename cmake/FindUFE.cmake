# - UFE finder module
# This module searches for a valid UFE installation.
# It searches for UFE's libraries and include header files
#
# Variables that will be defined:
# UFE_FOUND           Defined if a UFE installation has been detected
# UFE_LIBRARY         Path to UFE library
# UFE_INCLUDE_DIR     Path to the UFE's include directories
#

find_path(UFE_INCLUDE_DIR
        ufe/versionInfo.h
    HINTS
        "${UFE_INCLUDE_ROOT}"
        "${MAYA_DEVKIT_LOCATION}"
        "${MAYA_LOCATION}"
        "$ENV{MAYA_LOCATION}"
        "${MAYA_BASE_DIR}"
    PATH_SUFFIXES
        devkit/ufe/include
        include/
    DOC
        "UFE's headers path"
)

if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/ufe.h")
    foreach(_ufe_comp MAJOR_VERSION MINOR_VERSION PATCH_LEVEL)
        file(STRINGS
            "${UFE_INCLUDE_DIR}/ufe/ufe.h"
            _ufe_tmp
            REGEX "#define UFE_${_ufe_comp} .*$")
        if (NOT "${_ufe_tmp}" STREQUAL "")
            string(REGEX MATCHALL "[0-9]+" UFE_${_ufe_comp} ${_ufe_tmp})
        endif()
    endforeach()

    if(DEFINED UFE_PATCH_LEVEL)
        set(UFE_PATCH_VERSION "${UFE_PATCH_LEVEL}")
        set(UFE_VERSION ${UFE_MAJOR_VERSION}.${UFE_MINOR_VERSION}.${UFE_PATCH_VERSION})
    endif()
endif()

if(APPLE)
    find_path(UFE_LIBRARY_DIR
        libufe_${UFE_MAJOR_VERSION}.dylib
        HINTS
            "${UFE_LIB_ROOT}"
            "${MAYA_DEVKIT_LOCATION}"
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
            devkit/ufe/lib
            lib/
        DOC
            "UFE's libraries path"
    )
elseif(UNIX)
    find_path(UFE_LIBRARY_DIR
            libufe_${UFE_MAJOR_VERSION}.so
         HINTS
            "${UFE_LIB_ROOT}"
            "${MAYA_DEVKIT_LOCATION}"
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
            devkit/ufe/lib
            lib/
        DOC
            "UFE's libraries path"
    )
elseif(WIN32)
    find_path(UFE_LIBRARY_DIR
            ufe_${UFE_MAJOR_VERSION}.lib
         HINTS
            "${UFE_LIB_ROOT}"
            "${MAYA_DEVKIT_LOCATION}"
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
            devkit/ufe/lib
            lib/
        DOC
            "UFE's libraries path"
    )
endif()

foreach(UFE_LIB
    ufe_${UFE_MAJOR_VERSION})

    find_library(UFE_LIBRARY
        NAMES
            ${UFE_LIB}
        PATHS
            ${UFE_LIBRARY_DIR}
        NO_DEFAULT_PATH
    )

    if (UFE_LIBRARY)
        list(APPEND UFE_LIBRARIES ${UFE_LIBRARY})
    endif()
endforeach(UFE_LIB)

# handle the QUIETLY and REQUIRED arguments and set UFE_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(UFE
    REQUIRED_VARS
        UFE_INCLUDE_DIR
        UFE_LIBRARIES
    VERSION_VAR
        UFE_VERSION
)