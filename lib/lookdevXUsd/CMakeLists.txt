#
# Copyright 2024 Autodesk
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

project(lookdevXUsd)

add_library(${PROJECT_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME}
    PRIVATE
        LookdevXUsd.cpp
        ProxyShapeLookdevHandler.cpp
        UsdCapabilityHandler.cpp
        UsdClipboardHandler.cpp
        UsdDebugHandler.cpp
        UsdExtendedAttributeHandler.cpp
        UsdFileHandler.cpp
        UsdHierarchy.cpp
        UsdHierarchyHandler.cpp
        UsdLookdevHandler.cpp
        UsdMaterial.cpp
        UsdMaterialCommands.cpp
        UsdMaterialHandler.cpp
        UsdMaterialValidator.cpp
        UsdSceneItemUI.cpp
        UsdSceneItemUIHandler.cpp
        UsdSoloingHandler.cpp
        UsdUINodeGraphNode.cpp
        UsdUINodeGraphNodeHandler.cpp
        UsdMxVersionUpgrade.cpp
        Utils.cpp
)

set(HEADERS
    Export.h
    LookdevXUsd.h
    ProxyShapeLookdevHandler.h
    UsdCapabilityHandler.h
    UsdClipboardHandler.h
    UsdDebugHandler.h
    UsdExtendedAttributeHandler.h
    UsdFileHandler.h
    UsdHierarchy.h
    UsdHierarchyHandler.h
    UsdLookdevHandler.h
    UsdMaterial.h
    UsdMaterialCommands.h
    UsdMaterialHandler.h
    UsdMaterialValidator.h
    UsdSceneItemUI.h
    UsdSceneItemUIHandler.h
    UsdSoloingHandler.h
    UsdUINodeGraphNode.h
    UsdUINodeGraphNodeHandler.h
    UsdMxVersionUpgrade.h
    Utils.h
)

# -----------------------------------------------------------------------------
# Compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        LOOKDEVX_USD_SHARED
        PXR_VERSION=${PXR_VERSION}
        MFB_PACKAGE_NAME="${PROJECT_NAME}"
        MFB_ALT_PACKAGE_NAME="${PROJECT_NAME}"
        MFB_PACKAGE_MODULE="${PROJECT_NAME}"
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        # this flag is needed when building for Maya
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
)

mayaUsd_compile_config(${PROJECT_NAME})

mayaUsd_promoteHeaderList(
    HEADERS
        ${HEADERS}
    BASEDIR
        ${PROJECT_NAME}
)

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} 
    PUBLIC
        ${PXR_INCLUDE_DIRS}
        ${UFE_INCLUDE_DIR}
        ${LookdevXUfe_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}/include
)

# -----------------------------------------------------------------------------
# Versionned features
# -----------------------------------------------------------------------------
if (LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION)
    message(STATUS "Enabling legacy detection for MaterialX ${MaterialX_VERSION_MAJOR}.${MaterialX_VERSION_MINOR}")
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION=1
        MaterialX_VERSION_STRING="${MaterialX_VERSION_MAJOR}.${MaterialX_VERSION_MINOR}"
    )
endif()

# The ExtendedConnectionHandler and associated classes get removed from LookdevXUfe in UFE v8 / LookdevXUfe v3.
if (LOOKDEVXUFE_HAS_EXTENDED_CONNECTION_HANDLER)
    target_sources(${PROJECT_NAME}
    PRIVATE
        UsdComponentConnections.cpp
        UsdConnectionCommands.cpp
        UsdDeleteCommand.cpp
        UsdExtendedConnectionHandler.cpp
        UsdSceneItemOps.cpp
        UsdSceneItemOpsHandler.cpp
    )

    list(APPEND HEADERS
        UsdComponentConnections.h
        UsdConnectionCommands.h
        UsdDeleteCommand.h
        UsdExtendedConnectionHandler.h
        UsdSceneItemOps.h
        UsdSceneItemOpsHandler.h
    )

    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        LOOKDEVXUFE_HAS_EXTENDED_CONNECTION_HANDLER=1
    )
endif()

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        sdf
        sdr
        usdMtlx
        mayaUsdAPI
    PRIVATE
        usd
        usdShade
        usdUtils
        usdUI
        ${UFE_LIBRARY}
        ${LookdevXUfe_LIBRARY}
        usdUfe
)

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "lib")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if (IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_install_rpath(rpath ${PROJECT_NAME})
endif()

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------

# TODO - do we need to install the headers?
#install(FILES  ${HEADERS}
#    DESTINATION 
#        ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}
#)

install(TARGETS ${PROJECT_NAME}
    LIBRARY
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL
    )
endif()
