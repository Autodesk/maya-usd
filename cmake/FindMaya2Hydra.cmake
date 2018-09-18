# Simple module to find Maya2Hydra.

find_path(HDMAYA_INCLUDE_DIR hdmaya/delegates/delegate.h
          PATHS ${HDMAYA_ROOT}/include
          $ENV{HDMAYA_ROOT}/include
          DOC "HdMaya Include directory")

find_path(HDMAYA_LIBRARY_DIR libhdmaya.so
          PATHS ${HDMAYA_ROOT}/lib
          $ENV{HDMAYA_ROOT}/lib
          DOC "HdMaya Libraries directory")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    HDMAYA
    REQUIRED_VARS
    HDMAYA_INCLUDE_DIR
    HDMAYA_LIBRARY_DIR)
