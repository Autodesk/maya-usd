message("========== ADSK USD Plugin ==========")

set(INSTALL_DIR_SUFFIX MayaUSD)

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

add_subdirectory(plugin)
add_subdirectory(python)
