message("========== ADSK USD Plugin ==========")

set(INSTALL_DIR_SUFFIX MayaUSD)

list(APPEND CMAKE_MODULE_PATH
    ${PROJECT_SOURCE_DIR}/plugins/PXR_USDMaya/cmake/macros
)

include(Options)
include(Public)

#==============================================================================
# Packages 
#==============================================================================
find_package(MayaCoreUSD REQUIRED)
find_package(Maya REQUIRED)
find_package(USD REQUIRED)
include(${USD_CONFIG_FILE})

add_subdirectory(plugin)
add_subdirectory(python)
