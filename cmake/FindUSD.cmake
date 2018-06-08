# Simple module to find USD.

find_path(USD_INCLUDE_DIR pxr/pxr.h
          PATHS ${USD_ROOT}/include
          $ENV{USD_ROOT}/include
          DOC "USD Include directory")

find_path(USD_LIBRARY_DIR libusd.so
          PATHS ${USD_ROOT}/lib
          $ENV{USD_ROOT}/lib
          DOC "USD Libraries directory")

find_file(USD_GENSCHEMA
          names usdGenSchema
          PATHS ${USD_ROOT}/bin
          $ENV{USD_ROOT}/bin
          DOC "USD Gen schema application")

# USD Maya components

find_path(USD_MAYA_INCLUDE_DIR usdMaya/api.h
          PATHS ${USD_ROOT}/third_party/maya/include
          $ENV{USD_ROOT}/third_party/maya/include
          ${USD_MAYA_ROOT}/third_party/maya/include
          $ENV{USD_MAYA_ROOT}/third_party/maya/include
          DOC "USD Maya Include directory")

find_path(USD_MAYA_LIBRARY_DIR libusdMaya.so
          PATHS ${USD_ROOT}/third_party/maya/lib
          $ENV{USD_ROOT}/third_party/maya/lib
          ${USD_MAYA_ROOT}/third_party/maya/lib
          $ENV{USD_MAYA_ROOT}/third_party/maya/lib
          DOC "USD Maya Library directory")

# USD Katana components

find_path(USD_KATANA_INCLUDE_DIR usdKatana/api.h
          PATHS ${USD_ROOT}/third_party/katana/include
          $ENV{USD_ROOT}/third_party/katana/include
          ${USD_KATANA_ROOT}/third_party/katana/include
          $ENV{USD_KATANA_ROOT}/third_party/katana/include
          DOC "USD Katana Include directory")

find_path(USD_KATANA_LIBRARY_DIR libusdKatana.so
          PATHS ${USD_ROOT}/third_party/katana/lib
          $ENV{USD_ROOT}/third_party/katana/lib
          ${USD_KATANA_ROOT}/third_party/katana/lib
          $ENV{USD_KATANA_ROOT}/third_party/katana/lib
          DOC "USD Katana Library directory")

# USD Houdini components

find_path(USD_HOUDINI_INCLUDE_DIR gusd/api.h
          PATHS ${USD_ROOT}/third_party/houdini/include
          $ENV{USD_ROOT}/third_party/houdini/include
          ${USD_HOUDINI_ROOT}/third_party/houdini/include
          $ENV{USD_HOUDINI_ROOT}/third_party/houdini/include
          DOC "USD Houdini Include directory")

find_path(USD_HOUDINI_LIBRARY_DIR libgusd.so
          PATHS ${USD_ROOT}/third_party/houdini/lib
          $ENV{USD_ROOT}/third_party/houdini/lib
          ${USD_HOUDINI_ROOT}/third_party/houdini/lib
          $ENV{USD_HOUDINI_ROOT}/third_party/houdini/lib
          DOC "USD Houdini Library directory")

if(USD_INCLUDE_DIR AND EXISTS "${USD_INCLUDE_DIR}/pxr/pxr.h")
    foreach(_usd_comp MAJOR MINOR PATCH)
        file(STRINGS
             "${USD_INCLUDE_DIR}/pxr/pxr.h"
             _usd_tmp
             REGEX "#define PXR_${_usd_comp}_VERSION .*$")
        string(REGEX MATCHALL "[0-9]+" USD_${_usd_comp}_VERSION ${_usd_tmp})
    endforeach()
    set(USD_VERSION ${USD_MAJOR_VERSION}.${USD_MINOR_VERSION}.${USD_PATCH_VERSION})
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    USD
    REQUIRED_VARS
    USD_INCLUDE_DIR
    USD_LIBRARY_DIR
    USD_GENSCHEMA
    VERSION_VAR
    USD_VERSION)
