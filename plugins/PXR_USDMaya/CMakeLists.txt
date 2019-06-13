message("========== Pixar USD Plugin ==========")

#==============================================================================
# Modules and Definitions
#==============================================================================
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${PROJECT_SOURCE_DIR}/cmake/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/defaults
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros)

# CXXDefaults will set a variety of variables for the project.
# Consume them here. This is an effort to keep the most common
# build files readable.
include(CXXDefaults)
add_definitions(${_PXR_CXX_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${_PXR_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

include(Options)
include(Public)

#==============================================================================
# Packages 
#==============================================================================
find_package(Maya REQUIRED)

find_package(USD REQUIRED)
include(${USD_CONFIG_FILE})

# find_package(MayaCoreUSD REQUIRED)

pxr_toplevel_prologue()
    add_subdirectory(third_party/maya)
pxr_toplevel_epilogue()
