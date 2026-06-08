
find_path(USDUFE_INCLUDE_DIR
        usdUfe/usdUfe.h
    HINTS
        ${USDUFE_ROOT_DIR}/include
    DOC
        "USDUFE header path"
)

find_library(USDUFE_LIBRARY
    NAMES
        usdUfe
    HINTS
        ${USDUFE_ROOT_DIR}/lib
    DOC
        "USDUFE library"
    NO_DEFAULT_PATH
)

# Handle the QUIETLY and REQUIRED arguments and set UFE_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(USDUFE
    REQUIRED_VARS
        USDUFE_INCLUDE_DIR
        USDUFE_LIBRARY
)

if(USDUFE_FOUND)
    message(STATUS "UsdUfe include dir: ${USDUFE_INCLUDE_DIR}")
    message(STATUS "UsdUfe library: ${USDUFE_LIBRARY}")
endif()