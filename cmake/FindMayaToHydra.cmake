# Simple module to find MayaToHydra.

find_path(HDMAYA_INCLUDE_DIR hdmaya/delegates/delegate.h
          PATHS ${HDMAYA_ROOT}/include
          $ENV{HDMAYA_ROOT}/include
          DOC "HdMaya Include directory")

find_path(HDMAYA_LIBRARY_DIR libhdmaya.so
          PATHS ${HDMAYA_ROOT}/lib
          $ENV{HDMAYA_ROOT}/lib
          DOC "HdMaya Libraries directory")

if(HDMAYA_INCLUDE_DIR AND EXISTS "${HDMAYA_INCLUDE_DIR}/hdmaya/hdmaya.h")
    file(STRINGS
        "${HDMAYA_INCLUDE_DIR}/hdmaya/hdmaya.h"
        _hdmaya_ufe_tmp
        REGEX "#define HDMAYA_UFE_BUILD (.*)$")
    if (NOT "${_hdmaya_ufe_tmp}" STREQUAL "")
        string(REGEX REPLACE "#define HDMAYA_UFE_BUILD (.*)$" "\\1"
            HDMAYA_UFE_BUILD ${_hdmaya_ufe_tmp})
    endif()
endif()


include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    HDMAYA
    REQUIRED_VARS
    HDMAYA_INCLUDE_DIR
    HDMAYA_LIBRARY_DIR)
