#
# Copyright 2020 Autodesk
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
set(TARGET_NAME basePxrUsdPreviewSurface)

set(RESOURCES_INSTALL_PATH
    ${CMAKE_INSTALL_PREFIX}/lib/usd/${TARGET_NAME}/resources
)

set(LIBRARY_INSTALL_PATH
    ${CMAKE_INSTALL_PREFIX}/lib
)

add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
        usdPreviewSurface.cpp
        usdPreviewSurfacePlugin.cpp
        usdPreviewSurfaceReader.cpp
        usdPreviewSurfaceShadingNodeOverride.cpp
        usdPreviewSurfaceWriter.cpp
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME} 
    PRIVATE
        PXRUSDPREVIEWSURFACE_EXPORTS
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

mayaUsd_compile_config(${TARGET_NAME})

mayaUsd_promoteHeaderList( 
    HEADERS
        usdPreviewSurface.h
        usdPreviewSurfacePlugin.h
    BASEDIR
        ${TARGET_NAME}
)

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME}
    PRIVATE
        ${MAYA_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PUBLIC
        tf
    PRIVATE
        arch
        gf
        plug
        sdf
        vt
        usdShade
        ${MAYA_LIBRARIES}
        mayaUsd
)

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if(IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

#------------------------------------------------------------------------------
# install
#------------------------------------------------------------------------------
install(
    TARGETS ${TARGET_NAME}
    LIBRARY
    DESTINATION ${LIBRARY_INSTALL_PATH}
    ARCHIVE
    DESTINATION ${LIBRARY_INSTALL_PATH}
    RUNTIME
    DESTINATION ${LIBRARY_INSTALL_PATH}
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> 
        DESTINATION ${LIBRARY_INSTALL_PATH} OPTIONAL
    )
endif()
