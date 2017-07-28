# - Find Modo libraries
#
# Finds an installed Modo include files and libraries.
#
# Variables that will be defined:
# MODO_FOUND         Defined if a Modo installation has been detected
# MODO_LIBRARIES     Modo libraries and any dependant libraries
# MODO_INCLUDE_DIRS  Path to the Modo API include directories
# MODO_VERSION       Modo API version
find_path(MODO_BASE_DIR
    NAMES
        modo
    HINTS
        "${MODO_LOCATION}"
        "$ENV{MODO_LOCATION}"
)
find_library(MODO_LIBRARY
    NAMES
        libcommon.a
    HINTS
        "${MODO_LOCATION}"
        "$ENV{MODO_LOCATION}"
        "${MODO_BASE_DIR}"
    DOC
        "Modo library path"
)
find_path(MODO_INCLUDE_DIR 
    lxversion.h
    HINTS
        "${MODO_LOCATION}"
        "$ENV{MODO_LOCATION}"
        "${MODO_BASE_DIR}"
    PATH_SUFFIXES
        sdk/include/
    DOC
        "Modo headers path"
)

if(MODO_INCLUDE_DIR AND EXISTS "${MODO_INCLUDE_DIR}/lxversion.h")
    file(STRINGS
        ${MODO_INCLUDE_DIR}/lxversion.h
        TMP
        REGEX "#define LXs_VERSION_LIB .*$")
    string(REGEX MATCHALL "[v0-9.]+" MODO_VERSION ${TMP})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Modo
    REQUIRED_VARS
        MODO_BASE_DIR
        MODO_LIBRARY
        MODO_INCLUDE_DIR
    VERSION_VAR
        MODO_VERSION
)

set(MODO_LIBRARIES ${MODO_LIBRARY})
set(MODO_INCLUDE_DIRS ${MODO_INCLUDE_DIR})

