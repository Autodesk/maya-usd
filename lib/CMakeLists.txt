#
# Copyright 2019 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.12...3.17)

project(mayaUsd)

#==============================================================================
# packages
#==============================================================================
if(NOT DEFINED BOOST_ROOT)
    set(BOOST_ROOT ${PXR_USD_LOCATION})
endif()

find_package(Boost 
    COMPONENTS 
        filesystem 
    REQUIRED
)

#==============================================================================
# target
#==============================================================================
set(TARGET_NAME ${PROJECT_NAME})

add_library(${TARGET_NAME} SHARED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# -----------------------------------------------------------------------------
# compile definitions
# -----------------------------------------------------------------------------
set(compile_definitions_list
    MAYAUSD_MACROS_EXPORT
    MAYAUSD_CORE_EXPORT
    MFB_PACKAGE_NAME="${TARGET_NAME}"
    MFB_ALT_PACKAGE_NAME="${TARGET_NAME}"
    MFB_PACKAGE_MODULE=mayaUsd
)

if(IS_MACOSX)
    list(APPEND compile_definitions_list OSMac_)
endif()

target_compile_definitions(${TARGET_NAME} 
    PRIVATE
        ${compile_definitions_list}
)

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME} 
    PUBLIC
        ${MAYA_INCLUDE_DIRS}
        ${PXR_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}/include
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PUBLIC
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${MAYA_Foundation_LIBRARY}
        ${MAYA_OpenMayaAnim_LIBRARY}
        ${MAYA_OpenMaya_LIBRARY}
        ${MAYA_OpenMayaRender_LIBRARY}
        ${MAYA_OpenMayaUI_LIBRARY}
        ar
        gf
        hd
        hdx
        js
        kind
        plug
        sdf
        tf
        usd
        usdGeom
        usdImaging
        usdImagingGL
        usdLux
        usdRi
        usdShade
        usdSkel
        usdUtils
        vt
)

# handle rpath
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "plugin")
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../USD/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

if (IS_LINUX)
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../USD/lib64")
    endif()
endif()

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(TARGETS ${TARGET_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL)
endif()

#install top level plugInfo.json that includes the configured plugInfo.json
install(CODE
    "file(WRITE \"${CMAKE_INSTALL_PREFIX}/lib/usd/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)

# -----------------------------------------------------------------------------
# subdirectories
# -----------------------------------------------------------------------------
add_subdirectory(base)
add_subdirectory(listeners)
add_subdirectory(nodes)
add_subdirectory(utils)
add_subdirectory(render)
if(UFE_FOUND)
add_subdirectory(ufe)
endif()
