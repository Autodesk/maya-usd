#
# =======================================================================
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
# =======================================================================
#

cmake_minimum_required(VERSION 3.1.1)
project(mayaUsd)

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 OLD)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#==============================================================================
# Packages
#==============================================================================
set(BOOST_ROOT ${PXR_USD_LOCATION})
find_package(Boost COMPONENTS
                filesystem
                REQUIRED
)

#==============================================================================
# Library
#==============================================================================

set(LIBRARY_NAME mayaUsd)

list(APPEND mayaUsd_src
    #
    base/debugCodes.cpp
    #
    utils/colorSpace.cpp
    utils/query.cpp
    utils/util.cpp
    utils/utilFileSystem.cpp
    utils/stageCache.cpp
    #
    nodes/usdPrimProvider.cpp
    nodes/proxyShapeBase.cpp
    nodes/proxyShapePlugin.cpp
    nodes/stageData.cpp
    #
    listeners/stageNoticeListener.cpp
    listeners/notice.cpp
    listeners/proxyShapeNotice.cpp
    #
    render/vp2RenderDelegate/bboxGeom.cpp
    render/vp2RenderDelegate/debugCodes.cpp
    render/vp2RenderDelegate/render_param.cpp
    render/vp2RenderDelegate/instancer.cpp
    render/vp2RenderDelegate/draw_item.cpp
    render/vp2RenderDelegate/material.cpp
    render/vp2RenderDelegate/mesh.cpp
    render/vp2RenderDelegate/proxyRenderDelegate.cpp
    render/vp2RenderDelegate/render_delegate.cpp
    render/vp2RenderDelegate/sampler.cpp
    render/vp2RenderDelegate/tokens.cpp
    #
    render/vp2ShaderFragments/shaderFragments.cpp
    #
)

list(APPEND mayaUsdShaderFragments_xmls
    # Copied from pxrUsdPreviewSurface with change to support multiple lights
    render/vp2ShaderFragments/Float4ToFloatX.xml
    render/vp2ShaderFragments/Float4ToFloatY.xml
    render/vp2ShaderFragments/Float4ToFloatZ.xml
    render/vp2ShaderFragments/Float4ToFloatW.xml
    render/vp2ShaderFragments/lightingContributions.xml
    render/vp2ShaderFragments/opacityToTransparency.xml
    render/vp2ShaderFragments/scaledDiffusePassThrough.xml
    render/vp2ShaderFragments/scaledSpecularPassThrough.xml
    render/vp2ShaderFragments/usdPreviewSurfaceCombiner.xml
    render/vp2ShaderFragments/usdPreviewSurfaceLighting.xml
    render/vp2ShaderFragments/UsdPreviewSurface.xml
    # New fragments
    render/vp2ShaderFragments/FallbackCPVShader.xml
    render/vp2ShaderFragments/FallbackShader.xml
    render/vp2ShaderFragments/Float4ToFloat3.xml
    render/vp2ShaderFragments/Float4ToFloat4.xml
    render/vp2ShaderFragments/UsdPrimvarReader_float.xml
    render/vp2ShaderFragments/UsdPrimvarReader_float2.xml
    render/vp2ShaderFragments/UsdPrimvarReader_float3.xml
    render/vp2ShaderFragments/UsdPrimvarReader_float4.xml
    render/vp2ShaderFragments/UsdUVTexture.xml
    # USD plug info
    render/vp2ShaderFragments/plugInfo.json
)

if(UFE_FOUND)
    list(APPEND mayaUsd_src
        ufe/Global.cpp
        ufe/Utils.cpp
        ufe/ProxyShapeHandler.cpp
        ufe/ProxyShapeHierarchy.cpp
        ufe/ProxyShapeHierarchyHandler.cpp
        ufe/StagesSubject.cpp
        ufe/UsdHierarchy.cpp
        ufe/UsdHierarchyHandler.cpp
        ufe/UsdRootChildHierarchy.cpp
        ufe/UsdRotatePivotTranslateUndoableCommand.cpp
        ufe/UsdRotateUndoableCommand.cpp
        ufe/UsdScaleUndoableCommand.cpp
        ufe/UsdSceneItem.cpp
        ufe/UsdSceneItemOps.cpp
        ufe/UsdSceneItemOpsHandler.cpp
        ufe/UsdStageMap.cpp
        ufe/UsdTransform3d.cpp
        ufe/UsdTransform3dHandler.cpp
        ufe/UsdTranslateUndoableCommand.cpp
        ufe/UsdUndoDeleteCommand.cpp
        ufe/UsdUndoDuplicateCommand.cpp
        ufe/UsdUndoRenameCommand.cpp
        #
        ufe/private/Utils.cpp
    )

    if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
        list(APPEND mayaUsd_src
            ufe/UsdUndoCreateGroupCommand.cpp
            ufe/UsdAttribute.cpp
            ufe/UsdAttributes.cpp
            ufe/UsdAttributesHandler.cpp
        )
    endif()
endif()

list(APPEND mayaUsdBase_headers
    base/api.h
    base/debugCodes.h
)

list(APPEND mayaUsdUtils_headers
    utils/colorSpace.h
    utils/query.h
    utils/util.h
    utils/utilFileSystem.h
    utils/stageCache.h
    utils/query.h
)

list(APPEND mayaUsdNodes_headers
    nodes/usdPrimProvider.h
    nodes/proxyShapeBase.h
    nodes/proxyShapePlugin.h
    nodes/stageData.h
)

list(APPEND mayaUsdListeners_headers
    listeners/stageNoticeListener.h
    listeners/notice.h
    listeners/proxyShapeNotice.h
)

list(APPEND mayaUsdVP2RenderDelegate_headers
    render/vp2RenderDelegate/proxyRenderDelegate.h
)

if(UFE_FOUND)
    list(APPEND mayaUsdUfe_headers
        ufe/Global.h
        ufe/Utils.h
        ufe/ProxyShapeHandler.h
        ufe/ProxyShapeHierarchy.h
        ufe/ProxyShapeHierarchyHandler.h
        ufe/StagesSubject.h
        ufe/UsdHierarchy.h
        ufe/UsdHierarchyHandler.h
        ufe/UsdRootChildHierarchy.h
        ufe/UsdRotatePivotTranslateUndoableCommand.h
        ufe/UsdRotateUndoableCommand.h
        ufe/UsdScaleUndoableCommand.h
        ufe/UsdSceneItem.h
        ufe/UsdSceneItemOps.h
        ufe/UsdSceneItemOpsHandler.h
        ufe/UsdStageMap.h
        ufe/UsdTransform3d.h
        ufe/UsdTransform3dHandler.h
        ufe/UsdTranslateUndoableCommand.h
        ufe/UsdUndoDeleteCommand.h
        ufe/UsdUndoDuplicateCommand.h
        ufe/UsdUndoRenameCommand.h
    )

    if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
        list(APPEND mayaUsdUfe_headers
            ufe/UsdUndoCreateGroupCommand.h
            ufe/UsdAttribute.h
            ufe/UsdAttributes.h
            ufe/UsdAttributesHandler.h
        )
    endif()
endif()

add_library(${LIBRARY_NAME} SHARED
    ${mayaUsd_src}
)

set(compile_definitions_list
    MAYAUSD_MACROS_EXPORT
    MAYAUSD_CORE_EXPORT
    MFB_PACKAGE_NAME="${LIBRARY_NAME}"
    MFB_ALT_PACKAGE_NAME="${LIBRARY_NAME}"
    MFB_PACKAGE_MODULE=mayaUsd
)

if(UFE_FOUND)
    list(APPEND compile_definitions_list WANT_UFE_BUILD=1)
endif()
if(IS_MACOSX)
    list(APPEND compile_definitions_list OSMac_)
endif()

target_compile_definitions(${LIBRARY_NAME} PRIVATE
    ${compile_definitions_list}
)

# Some of the UFE classes are exporting STL classes which causes this warning.
if(UFE_FOUND AND MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251")
    string(REPLACE " " ";" REPLACED_FLAGS ${CMAKE_CXX_FLAGS})
    target_compile_options(${LIBRARY_NAME} INTERFACE ${REPLACED_FLAGS})
endif()

set(include_directories_list
    ${MAYA_INCLUDE_DIRS}
    ${PXR_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/include
)

if(UFE_FOUND)
    list(APPEND include_directories_list ${UFE_INCLUDE_DIR})
endif()

target_include_directories( ${LIBRARY_NAME} PUBLIC
    ${include_directories_list}
)

target_link_libraries(${LIBRARY_NAME}
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

if(UFE_FOUND)
    target_link_libraries(${LIBRARY_NAME} ${UFE_LIBRARY})
endif()

if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "plugin")
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../USD/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    mayaUsd_install_rpath(rpath ${LIBRARY_NAME})
endif()

if (IS_LINUX)
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../USD/lib64")
    endif()
endif()

install(TARGETS ${LIBRARY_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${LIBRARY_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL)
endif()

set_property(GLOBAL PROPERTY GLOBAL_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

# promote headers
mayaUsd_promoteMayaUsdHeader()
mayaUsd_promoteHeaderList(${mayaUsdBase_headers})
mayaUsd_promoteHeaderList(${mayaUsdUtils_headers})
mayaUsd_promoteHeaderList(${mayaUsdNodes_headers})
mayaUsd_promoteHeaderList(${mayaUsdListeners_headers})
mayaUsd_promoteHeaderList(${mayaUsdVP2RenderDelegate_headers})
if(UFE_FOUND)
    mayaUsd_promoteHeaderList(${mayaUsdUfe_headers})
endif()

# install public headers
install(FILES ${CMAKE_BINARY_DIR}/include/mayaUsd/mayaUsd.h
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/
)

install(FILES ${mayaUsdBase_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/base/
)

install(FILES ${mayaUsdUtils_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/utils/
)

install(FILES ${mayaUsdNodes_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/nodes/
)

install(FILES ${mayaUsdListeners_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/listeners/
)

install(FILES ${mayaUsdVP2RenderDelegate_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/render/vp2RenderDelegate
)

if(UFE_FOUND)
    install(FILES ${mayaUsdUfe_headers}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/ufe
    )
endif()

install(FILES ${mayaUsdShaderFragments_xmls}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd/mayaUsd_ShaderFragments/resources
)

#install top level plugInfo.json that includes the configured plugInfo.json
install(CODE
    "file(WRITE \"${CMAKE_INSTALL_PREFIX}/lib/usd/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)

if(UFE_FOUND)
    # UFE Python Bindings
    set(UFE_PYTHON_LIBRARY_NAME ufe)

    add_library(${UFE_PYTHON_LIBRARY_NAME}
        SHARED
        ufe/wrapUtils.cpp
    )

    target_compile_definitions(${UFE_PYTHON_LIBRARY_NAME}
        PRIVATE
            MFB_PACKAGE_NAME=${LIBRARY_NAME}
            MFB_ALT_PACKAGE_NAME=${LIBRARY_NAME}
            MFB_PACKAGE_MODULE=ufe
    )
    set_target_properties(${UFE_PYTHON_LIBRARY_NAME} 
        PROPERTIES 
            PREFIX ""
    )
    if(IS_WINDOWS)
        set_target_properties(${UFE_PYTHON_LIBRARY_NAME}
            PROPERTIES
                SUFFIX ".pyd"
        )
    elseif(IS_MACOSX)
        set_target_properties(${UFE_PYTHON_LIBRARY_NAME}
            PROPERTIES
                SUFFIX ".so"
        )
    endif()

    target_link_libraries(${UFE_PYTHON_LIBRARY_NAME}
        ${LIBRARY_NAME}
    )

    if(IS_MACOSX OR IS_LINUX)
        mayaUsd_init_rpath(rpath "${PROJECT_NAME}")
        mayaUsd_add_rpath(rpath "../..")
        mayaUsd_install_rpath(rpath ${UFE_PYTHON_LIBRARY_NAME})
    endif()

    install(TARGETS ${UFE_PYTHON_LIBRARY_NAME}
        LIBRARY
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}
        RUNTIME
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}
    )

    if(IS_WINDOWS)
        install(FILES $<TARGET_PDB_FILE:${UFE_PYTHON_LIBRARY_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME} OPTIONAL)
    endif()

    install(FILES ufe/__init__.py
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}
    )

endif()

if(IS_MACOSX)
    set(_macDef OSMac_)
endif()
