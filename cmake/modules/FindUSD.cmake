# Simple module to find USD.

# can use either ${USD_ROOT}, or ${USD_CONFIG_FILE} (which should be ${USD_ROOT}/pxrConfig.cmake)
# to find USD, defined as either Cmake var or env var
if (NOT DEFINED USD_ROOT AND NOT DEFINED ENV{USD_ROOT})
  if(DEFINED USD_CONFIG_FILE)
      get_filename_component(USD_ROOT "${USD_CONFIG_FILE}" DIRECTORY)
  elseif(DEFINED ENV{USD_CONFIG_FILE})
      get_filename_component(USD_ROOT "$ENV{USD_CONFIG_FILE}" DIRECTORY)
  endif ()
endif ()

# On a system with an existing USD /usr/local installation added to the system
# PATH, use of PATHS in find_path incorrectly causes the existing USD
# installation to be found.  As per
# https://cmake.org/cmake/help/v3.4/command/find_path.html
# and
# https://cmake.org/pipermail/cmake/2010-October/040460.html
# HINTS get searched before system paths, which produces the desired result.
find_path(USD_INCLUDE_DIR pxr/pxr.h
          HINTS ${USD_ROOT}/include
                $ENV{USD_ROOT}/include
          DOC "USD Include directory")

if(WIN32)
    find_path(USD_LIBRARY_DIR libusd.dll
            PATHS ${USD_ROOT}/lib
                  $ENV{USD_ROOT}/lib
            DOC "USD Libraries directory")
elseif(UNIX)
    find_path(USD_LIBRARY_DIR libusd.so
            PATHS ${USD_ROOT}/lib
                  $ENV{USD_ROOT}/lib
            DOC "USD Libraries directory")
endif()

find_file(USD_GENSCHEMA
          names usdGenSchema
          PATHS ${USD_ROOT}/bin
                $ENV{USD_ROOT}/bin
          DOC "USD Gen schema application")

find_file(USD_CONFIG_FILE
          names pxrConfig.cmake
          PATHS ${USD_ROOT}
                $ENV{USD_ROOT}
          DOC "USD cmake configuration file")

# USD Maya components

find_path(USD_MAYA_INCLUDE_DIR usdMaya/api.h
          PATHS ${USD_ROOT}/third_party/maya/include
                $ENV{USD_ROOT}/third_party/maya/include
                ${USD_MAYA_ROOT}/third_party/maya/include
                $ENV{USD_MAYA_ROOT}/third_party/maya/include
          DOC "USD Maya Include directory")

find_library(USD_MAYA_LIBRARY usdMaya
          PATHS ${USD_ROOT}/third_party/maya/lib
                $ENV{USD_ROOT}/third_party/maya/lib
                ${USD_MAYA_ROOT}/third_party/maya/lib
                $ENV{USD_MAYA_ROOT}/third_party/maya/lib
          DOC "USD Maya Library")
          
get_filename_component(USD_MAYA_LIBRARY_DIR "${USD_MAYA_LIBRARY}" DIRECTORY)
option(USD_MAYA_LIBRARY_DIR "USD Maya Library directory" "${USD_MAYA_LIBRARY_DIR}")

# USD Katana components

find_path(USD_KATANA_INCLUDE_DIR usdKatana/api.h
          PATHS ${USD_ROOT}/third_party/katana/include
                $ENV{USD_ROOT}/third_party/katana/include
                ${USD_KATANA_ROOT}/third_party/katana/include
                $ENV{USD_KATANA_ROOT}/third_party/katana/include
          DOC "USD Katana Include directory")

find_library(USD_KATANA_LIBRARY usdKatana
          PATHS ${USD_ROOT}/third_party/katana/lib
                $ENV{USD_ROOT}/third_party/katana/lib
                ${USD_KATANA_ROOT}/third_party/katana/lib
                $ENV{USD_KATANA_ROOT}/third_party/katana/lib
          DOC "USD Katana Library")

get_filename_component(USD_KATANA_LIBRARY_DIR "${USD_KATANA_LIBRARY}" DIRECTORY)
option(USD_KATANA_LIBRARY_DIR "USD Katana Library directory" "${USD_KATANA_LIBRARY_DIR}")

# USD Houdini components

find_path(USD_HOUDINI_INCLUDE_DIR gusd/api.h
          PATHS ${USD_ROOT}/third_party/houdini/include
                $ENV{USD_ROOT}/third_party/houdini/include
                ${USD_HOUDINI_ROOT}/third_party/houdini/include
                $ENV{USD_HOUDINI_ROOT}/third_party/houdini/include
          DOC "USD Houdini Include directory")

find_library(USD_HOUDINI_LIBRARY gusd
          PATHS ${USD_ROOT}/third_party/houdini/lib
                $ENV{USD_ROOT}/third_party/houdini/lib
                ${USD_HOUDINI_ROOT}/third_party/houdini/lib
                $ENV{USD_HOUDINI_ROOT}/third_party/houdini/lib
          DOC "USD Houdini Library")

get_filename_component(USD_HOUDINI_LIBRARY_DIR "${USD_HOUDINI_LIBRARY}" DIRECTORY)
option(USD_HOUDINI_LIBRARY_DIR "USD Houdini Library directory" "${USD_HOUDINI_LIBRARY_DIR}")


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
    USD_CONFIG_FILE
    VERSION_VAR
    USD_VERSION)
