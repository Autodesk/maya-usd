# Simple module to find USD.

# can use either ${USD_ROOT}, or ${USD_CONFIG_FILE} (which should be ${USD_ROOT}/pxrConfig.cmake)
# to find USD, defined as either Cmake var or env var
if (NOT DEFINED USD_ROOT)
  if (DEFINED ENV{USD_ROOT})
      set(USD_ROOT "$ENV{USD_ROOT}")
  elseif(DEFINED USD_CONFIG_FILE)
      get_filename_component(USD_ROOT "${USD_CONFIG_FILE}" DIRECTORY)
  elseif(DEFINED ENV{USD_CONFIG_FILE})
      get_filename_component(USD_ROOT "$ENV{USD_CONFIG_FILE}" DIRECTORY)
  endif ()
endif ()

find_path(USD_INCLUDE_DIR pxr/pxr.h
          PATHS ${USD_ROOT}/include
          DOC "USD Include directory")

find_path(USD_LIBRARY_DIR libusd.so
          PATHS ${USD_ROOT}/lib
          DOC "USD Librarires directory")

find_file(USD_GENSCHEMA
          names usdGenSchema
          PATHS ${USD_ROOT}/bin
          DOC "USD Gen schema application")

find_file(USD_CONFIG_FILE
        names pxrConfig.cmake
        PATHS ${USD_ROOT}
        DOC "USD cmake configuration file")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    USD
    REQUIRED_VARS
    USD_INCLUDE_DIR
    USD_LIBRARY_DIR
    USD_GENSCHEMA
    USD_CONFIG_FILE)
