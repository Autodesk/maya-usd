message("========== ADSK USD Plugin ==========")

set(INSTALL_DIR_SUFFIX MayaUSD)

list(APPEND CMAKE_MODULE_PATH
    ${PROJECT_SOURCE_DIR}/plugins/PXR_USDMaya/cmake/macros
)

include(Options)
include(Public)

# Setup the Rpath
if (APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
endif()

#==============================================================================
# Packages 
#==============================================================================
find_package(MayaCoreUSD REQUIRED)
find_package(Maya REQUIRED)
find_package(USD REQUIRED)
include(${USD_CONFIG_FILE})

add_subdirectory(plugin)
add_subdirectory(python)
