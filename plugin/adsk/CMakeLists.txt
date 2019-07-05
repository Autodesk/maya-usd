message("========== ADSK USD Plugin ==========")

set(INSTALL_DIR_SUFFIX MayaUSD)

#==============================================================================
# Packages 
#==============================================================================
find_package(MayaUSD REQUIRED)
find_package(Maya REQUIRED)

add_subdirectory(plugin)
add_subdirectory(python)
