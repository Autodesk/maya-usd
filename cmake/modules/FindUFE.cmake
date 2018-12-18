# - UFE finder module
# This module searches for a valid UFE installation.
# It searches for UFE's libraries and include header files
#
# Variables that will be defined:
# UFE_FOUND           Defined if a UFE installation has been detected
# UFE_LIBRARY         Path to UFE library
# UFE_INCLUDE_DIR     Path to the UFE's include directories
#

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

message(STATUS "UFE Include directory: ${UFE_INCLUDE_DIR}")
message(STATUS "UFE Library directory: ${UFE_LIBRARY_DIR}")

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
)