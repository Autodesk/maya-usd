message("========== Pixar USD Plugin ==========")

#==============================================================================
# Modules and Definitions
#==============================================================================
set(INSTALL_DIR_SUFFIX plugin/pxr)
set(PXR_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR_SUFFIX})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${PROJECT_SOURCE_DIR}/cmake/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros
)

include(Options)
include(Public)

pxr_setup_python()

#==============================================================================
# Packages 
#==============================================================================
pxr_toplevel_prologue()
    add_subdirectory(maya)
pxr_toplevel_epilogue()
