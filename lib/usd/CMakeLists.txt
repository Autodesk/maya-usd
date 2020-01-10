#==============================================================================
# Configuration hdMaya 
#==============================================================================
set(MTOH_CXX_STD "11"
    CACHE
    STRING
    "Which C++ standard / version to use, as an integer - ie, 14 means use c++14."
)

option(INSTALL_MAYA_MOD "Installs a maya module file for the plugin." OFF)
option(HDMAYA_BUILD_TESTS "Build the maya-to-hydra tests" OFF)
option(BUILD_DOCS "Whether or not to build the documentation." OFF)

if(CMAKE_WANT_UFE_BUILD)
    find_package(UFE QUIET)
    if(UFE_FOUND)
        message(STATUS "Building with UFE ${UFE_VERSION} features enabled.")
        include_directories(${UFE_INCLUDE_DIR})
        add_definitions(-DWANT_UFE_BUILD)
    else()
        message(STATUS "UFE not found. UFE features will be disabled.")
    endif()
endif()

# promote headers
list(APPEND schemas_headers
    schemas/MayaReference.h
    schemas/api.h
    schemas/tokens.h
)

mayaUsd_promoteHeaderList(${schemas_headers})

add_subdirectory(hdMaya)
add_subdirectory(translators)
add_subdirectory(schemas)
