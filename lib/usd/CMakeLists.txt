#==============================================================================
# Configuration hdMaya 
#==============================================================================

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

if(BUILD_HDMAYA)
    add_subdirectory(hdMaya)
endif()
add_subdirectory(translators)
add_subdirectory(schemas)
