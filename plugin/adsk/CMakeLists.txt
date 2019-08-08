message("========== ADSK USD Plugin ==========")

set(INSTALL_DIR_SUFFIX plugin/adsk)

#==============================================================================
# Packages 
#==============================================================================
find_package(MayaUSD REQUIRED)
find_package(Maya REQUIRED)
find_package(USD REQUIRED)
include(${USD_CONFIG_FILE})
include_directories(${USD_INCLUDE_DIR})

add_subdirectory(plugin)
