message("========== ADSK USD Plugin ==========")

set(INSTALL_DIR_SUFFIX plugin/adsk)

#==============================================================================
# Packages 
#==============================================================================
find_package(MayaUSD REQUIRED)
find_package(Maya REQUIRED)
find_package(USD REQUIRED)
include(${USD_CONFIG_FILE})

add_subdirectory(plugin)
add_subdirectory(scripts)
