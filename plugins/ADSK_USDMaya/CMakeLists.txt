message("========== ADSK USD Plugin ==========")

#==============================================================================
# Packages 
#==============================================================================
find_package(MayaCoreUSD REQUIRED)
find_package(Maya REQUIRED)
find_package(USD REQUIRED)
include(${USD_CONFIG_FILE})

add_subdirectory(plugin)
add_subdirectory(python)
