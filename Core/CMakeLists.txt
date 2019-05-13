cmake_minimum_required(VERSION 3.1.1)

project(mayaUsd)

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 OLD)
endif()

#==============================================================================
# Modules
#==============================================================================
list(APPEND CMAKE_MODULE_PATH
    ${PROJECT_SOURCE_DIR}/../cmake/defaults
    ${PROJECT_SOURCE_DIR}/../cmake/modules
)

#==============================================================================
# 
#==============================================================================
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(CXXDefaults)

if(WIN32)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} ${_PXR_CXX_FLAGS}"
    )
else()
    set(CMAKE_CXX_FLAGS
        -std=c++11
        -msse3
        "${CMAKE_CXX_FLAGS} ${_PXR_CXX_FLAGS}"
    )
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

#==============================================================================
# 
#==============================================================================
add_subdirectory(include)
add_subdirectory(src)