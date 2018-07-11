# Simple module to find AL_USDMaya.

find_path(ALUSDMAYA_INCLUDE_DIR AL/usdmaya/nodes/ProxyShape.h
          PATHS ${ALUSDMAYA_ROOT}/include
          ${AL_USDMAYA_ROOT}/include
          $ENV{ALUSDMAYA_ROOT}/include
          $ENV{AL_USDMAYA_ROOT}/include
          DOC "AL_USDMaya Include directory")

find_path(ALUSDMAYA_LIBRARY_DIR libAL_USDMaya.so
          PATHS ${ALUSDMAYA_ROOT}/lib
          ${AL_USDMAYA_ROOT}/lib
          $ENV{ALUSDMAYA_ROOT}/lib
          $ENV{AL_USDMAYA_ROOT}/lib
          DOC "AL_USDMaya Libraries directory")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    ALUSDMAYA
    REQUIRED_VARS
    ALUSDMAYA_INCLUDE_DIR
    ALUSDMAYA_LIBRARY_DIR)
