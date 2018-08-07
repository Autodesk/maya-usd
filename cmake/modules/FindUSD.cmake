
# Simple module to find USD.

if (EXISTS "$ENV{USD_ROOT}")
    set(USD_ROOT $ENV{USD_ROOT})
endif ()

find_path(PXR_INCLUDE_DIRS pxr/pxr.h
          PATHS ${USD_ROOT}/include
          DOC "USD Include directory")

find_library(USD_LIBRARY
			NAMES usd
			HINTS "${USD_ROOT}/lib"
			DOC "USD Librarires directory"
)

get_filename_component(USD_LIBRARY_DIR ${USD_LIBRARY} DIRECTORY)

find_file(USD_GENSCHEMA
          names usdGenSchema
          PATHS ${USD_ROOT}/bin
          DOC "USD Gen schema application")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    USD
    REQUIRED_VARS
    PXR_INCLUDE_DIRS
    USD_LIBRARY_DIR
    USD_GENSCHEMA)