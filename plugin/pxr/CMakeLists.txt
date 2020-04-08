message("========== Pixar USD Plugin ==========")

#==============================================================================
# Modules and Definitions
#==============================================================================
set(INSTALL_DIR_SUFFIX plugin/pxr)
set(PXR_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR_SUFFIX})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${PROJECT_SOURCE_DIR}/cmake/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/defaults
)

include(Version)
include(Options)
include(Public)
pxr_setup_python()

#------------------------------------------------------------------------------
# compiler configuration
#------------------------------------------------------------------------------
# CXXDefaults will set a variety of variables for the project.
# Consume them here. This is an effort to keep the most common
# build files readable.
include(CXXDefaults)
add_definitions(${_PXR_CXX_DEFINITIONS} -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
set(CMAKE_CXX_FLAGS "${_PXR_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS
        -msse3
        "${CMAKE_CXX_FLAGS} ${_PXR_CXX_FLAGS}"
    )
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

#==============================================================================
# subdirectory
#==============================================================================
pxr_toplevel_prologue()
    add_subdirectory(maya)
pxr_toplevel_epilogue()
