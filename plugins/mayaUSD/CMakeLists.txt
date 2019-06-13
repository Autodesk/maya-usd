message("========== Building MayaUSD Plugin ==========")

#==============================================================================
# Packages 
#==============================================================================
find_package(MayaCoreUSD REQUIRED)
find_package(Maya REQUIRED)

add_subdirectory(plugin)
add_subdirectory(python)
