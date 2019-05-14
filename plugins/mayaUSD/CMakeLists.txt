message("========== Building mayaUsdPlugin ... ==========")

#==============================================================================
# Definitions
#==============================================================================
if (APPLE)
   add_definitions(-DOSMac_ -DMAC_PLUGIN)
endif()

#==============================================================================
# Packages 
#==============================================================================
find_package(MayaCoreUSD REQUIRED)
find_package(Maya REQUIRED)

add_subdirectory(plugin)
add_subdirectory(python)
