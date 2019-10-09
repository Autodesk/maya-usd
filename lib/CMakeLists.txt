#
# =======================================================================
# Copyright 2019 Autodesk, Inc. All rights reserved.
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
    fileio/registryHelper.cpp
    fileio/primReaderContext.cpp
    fileio/primReaderArgs.cpp
    fileio/primReader.cpp
    fileio/primReaderRegistry.cpp
    fileio/functorPrimReader.cpp
    fileio/fallbackPrimReader.cpp
    fileio/primWriterContext.cpp
    fileio/primWriterArgs.cpp
    fileio/primWriter.cpp
    fileio/primWriterRegistry.cpp
    fileio/functorPrimWriter.cpp
    fileio/writeJobContext.cpp
    fileio/instancedNodeWriter.cpp
    fileio/transformWriter.cpp
    fileio/shaderWriter.cpp
    fileio/primUpdater.cpp
    fileio/primUpdaterContext.cpp
    fileio/primUpdaterRegistry.cpp
    #
    fileio/chaser/chaser.cpp
    fileio/chaser/chaserRegistry.cpp
    #
    fileio/jobs/jobArgs.cpp
    fileio/jobs/readJob.cpp
    fileio/jobs/writeJob.cpp
    fileio/jobs/modelKindProcessor.cpp
    #
    fileio/translators/translatorUtil.cpp
    fileio/translators/translatorXformable.cpp
    fileio/translators/translatorXformable_decompose.cpp
    fileio/translators/translatorPrim.cpp
    fileio/translators/translatorGprim.cpp
    fileio/translators/skelBindingsProcessor.cpp
    fileio/translators/translatorMaterial.cpp
    fileio/translators/translatorCamera.cpp
    fileio/translators/translatorCurves.cpp
    fileio/translators/translatorMesh.cpp
    fileio/translators/translatorMesh_SubDiv.cpp
    fileio/translators/translatorMesh_PrimVars.cpp
    fileio/translators/translatorNurbsPatch.cpp
    fileio/translators/translatorRfMLight.cpp
    fileio/translators/translatorSkel.cpp
    fileio/translators/translatorMayaReference.cpp
    #
    fileio/shading/shadingModeDisplayColor.cpp
    fileio/shading/shadingModeExporterContext.cpp
    fileio/shading/shadingModeExporter.cpp
    fileio/shading/shadingModeImporter.cpp
    fileio/shading/shadingModePxrRis.cpp
    fileio/shading/shadingModeRegistry.cpp
    fileio/shading/shadingModeUseRegistry.cpp
    #
    fileio/utils/adaptor.cpp
    fileio/utils/readUtil.cpp
    fileio/utils/userTaggedAttribute.cpp
    fileio/utils/writeUtil.cpp
    fileio/utils/shadingUtil.cpp
    fileio/utils/meshUtil.cpp
    fileio/utils/roundTripUtil.cpp
    fileio/utils/xformStack.cpp
    #
    utils/colorSpace.cpp
    utils/diagnosticDelegate.cpp
    utils/query.cpp
    utils/util.cpp
    utils/utilFileSystem.cpp
    utils/blockSceneModificationContext.cpp
    utils/stageCache.cpp
    utils/query.cpp
    utils/undoHelperCommand
    #
    nodes/pointBasedDeformerNode.cpp
    nodes/hdImagingShape.cpp
    nodes/usdPrimProvider.cpp
    nodes/proxyShapeBase.cpp
    nodes/proxyShapePlugin.cpp
    nodes/stageData.cpp
    nodes/stageNode.cpp
    #
    listeners/stageNoticeListener.cpp
    listeners/notice.cpp
    listeners/proxyShapeNotice.cpp
    #
    render/vp2RenderDelegate/debugCodes.cpp
    render/vp2RenderDelegate/render_param.cpp
    render/vp2RenderDelegate/instancer.cpp
    render/vp2RenderDelegate/draw_item.cpp
    render/vp2RenderDelegate/material.cpp
    render/vp2RenderDelegate/mesh.cpp
    render/vp2RenderDelegate/proxyRenderDelegate.cpp
    render/vp2RenderDelegate/render_delegate.cpp
    render/vp2RenderDelegate/sampler.cpp
    #
    render/vp2ShaderFragments/shaderFragments.cpp
    #
    render/pxrUsdMayaGL/batchRenderer.cpp
    render/pxrUsdMayaGL/debugCodes.cpp
    render/pxrUsdMayaGL/hdImagingShapeDrawOverride.cpp
    render/pxrUsdMayaGL/hdImagingShapeUI.cpp
    render/pxrUsdMayaGL/hdRenderer.cpp
    render/pxrUsdMayaGL/proxyDrawOverride.cpp
    render/pxrUsdMayaGL/proxyShapeUI.cpp
    render/pxrUsdMayaGL/sceneDelegate.cpp
    render/pxrUsdMayaGL/shapeAdapter.cpp
    render/pxrUsdMayaGL/softSelectHelper.cpp
    render/pxrUsdMayaGL/usdProxyShapeAdapter.cpp
    render/pxrUsdMayaGL/userData.cpp
    render/pxrUsdMayaGL/instancerImager.cpp
    render/pxrUsdMayaGL/instancerShapeAdapter.cpp
    render/pxrUsdMayaGL/proxyShapeDelegate.cpp
    #
    render/px_vp20/glslProgram.cpp
    render/px_vp20/utils.cpp
    render/px_vp20/utils_legacy.cpp
)

list(APPEND mayaUsdShaderFragments_xmls
    # Copied from pxrUsdPreviewSurface with change to support multiple lights
    render/vp2ShaderFragments/float4ToFloatX.xml
    render/vp2ShaderFragments/float4ToFloatY.xml
    render/vp2ShaderFragments/float4ToFloatZ.xml
    render/vp2ShaderFragments/float4ToFloatW.xml
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
    render/vp2ShaderFragments/float4ToFloat3.xml
    render/vp2ShaderFragments/float4ToFloat4.xml
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

list(APPEND mayaUsdFileio_headers
    fileio/registryHelper.h
    fileio/primReaderContext.h
    fileio/primReaderArgs.h
    fileio/primReader.h
    fileio/primReaderRegistry.h
    fileio/functorPrimReader.h
    fileio/fallbackPrimReader.h
    fileio/primWriterContext.h
    fileio/primWriterArgs.h
    fileio/primWriter.h
    fileio/primWriterRegistry.h
    fileio/functorPrimWriter.h
    fileio/writeJobContext.h
    fileio/instancedNodeWriter.h
    fileio/transformWriter.h
    fileio/shaderWriter.h
    fileio/primUpdater.h
    fileio/primUpdaterContext.h
    fileio/primUpdaterRegistry.h
)

list(APPEND mayaUsdChaser_headers
    fileio/chaser/chaser.h
    fileio/chaser/chaserRegistry.h
)

list(APPEND mayaUsdJobs_headers
    fileio/jobs/jobArgs.h
    fileio/jobs/readJob.h
    fileio/jobs/writeJob.h
    fileio/jobs/modelKindProcessor.h
)

list(APPEND mayaUsdShading_headers
    fileio/shading/shadingModeExporterContext.h
    fileio/shading/shadingModeExporter.h
    fileio/shading/shadingModeImporter.h
    fileio/shading/shadingModeRegistry.h
)

list(APPEND mayaUsdUtilsIO_headers
    fileio/utils/adaptor.h
    fileio/utils/readUtil.h
    fileio/utils/userTaggedAttribute.h
    fileio/utils/writeUtil.h
    fileio/utils/shadingUtil.h
    fileio/utils/meshUtil.h
    fileio/utils/roundTripUtil.h
    fileio/utils/xformStack.h
)

list(APPEND mayaUsdTranslators_headers
    fileio/translators/translatorUtil.h
    fileio/translators/translatorXformable.h
    fileio/translators/translatorPrim.h
    fileio/translators/translatorGprim.h
    fileio/translators/skelBindingsProcessor.h
    fileio/translators/translatorMaterial.h
    fileio/translators/translatorCamera.h
    fileio/translators/translatorCurves.h
    fileio/translators/translatorMesh.h
    fileio/translators/translatorNurbsPatch.h
    fileio/translators/translatorRfMLight.h
    fileio/translators/translatorSkel.h
    fileio/translators/translatorMayaReference.h
)

list(APPEND mayaUsdUtils_headers
    utils/colorSpace.h
    utils/diagnosticDelegate.h
    utils/query.h
    utils/util.h
    utils/utilFileSystem.h
    utils/blockSceneModificationContext.h
    utils/stageCache.h
    utils/query.h
    utils/undoHelperCommand.h
)

list(APPEND mayaUsdNodes_headers
    nodes/pointBasedDeformerNode.h
    nodes/hdImagingShape.h
    nodes/usdPrimProvider.h
    nodes/proxyShapeBase.h
    nodes/proxyShapePlugin.h
    nodes/stageData.h
    nodes/stageNode.h
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

list(APPEND mayaUsdPxrUsdMayaGL_headers
    render/pxrUsdMayaGL/batchRenderer.h
    render/pxrUsdMayaGL/debugCodes.h
    render/pxrUsdMayaGL/hdImagingShapeUI.h
    render/pxrUsdMayaGL/hdImagingShapeDrawOverride.h
    render/pxrUsdMayaGL/instancerImager.h
    render/pxrUsdMayaGL/instancerShapeAdapter.h
    render/pxrUsdMayaGL/proxyShapeUI.h
    render/pxrUsdMayaGL/proxyDrawOverride.h
    render/pxrUsdMayaGL/renderParams.h
    render/pxrUsdMayaGL/sceneDelegate.h
    render/pxrUsdMayaGL/shapeAdapter.h
    render/pxrUsdMayaGL/softSelectHelper.h
    render/pxrUsdMayaGL/usdProxyShapeAdapter.h
    render/pxrUsdMayaGL/userData.h
)

list(APPEND mayaUsdPxVP20_headers
    render/px_vp20/utils.h
    render/px_vp20/utils_legacy.h
)

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
mayaUsd_promoteHeaderList(${mayaUsdFileio_headers})
mayaUsd_promoteHeaderList(${mayaUsdChaser_headers})
mayaUsd_promoteHeaderList(${mayaUsdJobs_headers})
mayaUsd_promoteHeaderList(${mayaUsdUtilsIO_headers})
mayaUsd_promoteHeaderList(${mayaUsdShading_headers})
mayaUsd_promoteHeaderList(${mayaUsdTranslators_headers})
mayaUsd_promoteHeaderList(${mayaUsdUtils_headers})
mayaUsd_promoteHeaderList(${mayaUsdNodes_headers})
mayaUsd_promoteHeaderList(${mayaUsdListeners_headers})
mayaUsd_promoteHeaderList(${mayaUsdVP2RenderDelegate_headers})
if(UFE_FOUND)
    mayaUsd_promoteHeaderList(${mayaUsdUfe_headers})
endif()
mayaUsd_promoteHeaderList(${mayaUsdPxrUsdMayaGL_headers})

# install public headers
install(FILES ${CMAKE_BINARY_DIR}/include/mayaUsd/mayaUsd.h
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/
)

install(FILES ${mayaUsdBase_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/base/
)

install(FILES ${mayaUsdFileio_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/fileio/
)

install(FILES ${mayaUsdChaser_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/fileio/chaser/
)

install(FILES ${mayaUsdJobs_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/fileio/jobs/
)

install(FILES ${mayaUsdUtilsIO_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/fileio/utils/
)

install(FILES ${mayaUsdShading_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/fileio/shading/
)

install(FILES ${mayaUsdTranslators_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/fileio/translators/
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

install(FILES ${mayaUsdPxrUsdMayaGL_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/render/pxrUsdMayaGL
)

install(FILES ${mayaUsdShaderFragments_xmls}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd/mayaUsd_ShaderFragments/resources
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

    if (CMAKE_UFE_V2_FEATURES_AVAILABLE)
        # Maya Attribute Editor python template files
        install(FILES ae/mesh/ae_template.py
             DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd/nodes/mesh
        )
        # We need an empty __init__.py file in each subfolder so it is considered a python module.
        set(_INIT_PY_DEST ${CMAKE_INSTALL_PREFIX}/lib/python/)
        foreach(_SUBDIR ufe_ae usd nodes mesh)
            string(APPEND _INIT_PY_DEST "/${_SUBDIR}")
            install(FILES ae/__init__.py
                 DESTINATION ${_INIT_PY_DEST}
            )
        endforeach()
    endif()

endif()

# Python extension library for mayaUsd library bindings.  Capitalize leading
# character of library name.  Pixar's TF_WRAP_MODULE macro expects the library
# name to be prefixed with _, but adds an underscore to the package name when
# creating the Python extension module initialization function (see Pixar's
# pyModule.h).  Create a package name without the leading underscore, and
# prefix our library name with a leading underscore, to match this convention.
string(TOUPPER "${LIBRARY_NAME}" LIBRARY_NAME_CAPS)
string(SUBSTRING "${LIBRARY_NAME}" 1 -1 LIBRARY_NAME_END)
string(SUBSTRING "${LIBRARY_NAME_CAPS}" 0 1 LIBRARY_NAME_BEGIN)
string(CONCAT PACKAGE_NAME "Py" "${LIBRARY_NAME_BEGIN}" "${LIBRARY_NAME_END}")
string(CONCAT PYTHON_LIBRARY_NAME "_" "${PACKAGE_NAME}")

add_library(${PYTHON_LIBRARY_NAME}
    SHARED
    python/module.cpp
    python/wrapAdaptor.cpp
    python/wrapBlockSceneModificationContext.cpp
    python/wrapColorSpace.cpp
    python/wrapDiagnosticDelegate.cpp
    python/wrapMeshUtil.cpp
    python/wrapQuery.cpp
    python/wrapReadUtil.cpp
    python/wrapRoundTripUtil.cpp
    python/wrapStageCache.cpp
    python/wrapUserTaggedAttribute.cpp
    python/wrapWriteUtil.cpp
    python/wrapXformStack.cpp
)

target_compile_definitions(${PYTHON_LIBRARY_NAME}
    PRIVATE
        MFB_PACKAGE_NAME=${PACKAGE_NAME}
        MFB_ALT_PACKAGE_NAME=${PACKAGE_NAME}
        MFB_PACKAGE_MODULE=${PROJECT_NAME}
)
set_target_properties(${PYTHON_LIBRARY_NAME} 
    PROPERTIES 
        PREFIX ""
)
if(IS_WINDOWS)
set_target_properties(${PYTHON_LIBRARY_NAME}
    PROPERTIES
        SUFFIX ".pyd"
)
elseif(IS_MACOSX)
    set_target_properties(${UFE_PYTHON_LIBRARY_NAME}
        PROPERTIES
            SUFFIX ".so"
    )
endif()

target_link_libraries(${PYTHON_LIBRARY_NAME}
    ${LIBRARY_NAME}
)

# We will use the following Python package structure:
#
# mayaUsd: top-level package
# mayaUsd.lib: Python wrappers for mayaUsd library functionality.
#
# We re-map the Python library namespace to mayaUsd.lib through a simple
# __init__.py.

set(PYLIB_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}/lib)

if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "lib")
    mayaUsd_add_rpath(rpath "../../..")
    mayaUsd_install_rpath(rpath ${PYTHON_LIBRARY_NAME})
endif()

install(TARGETS ${PYTHON_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${PYLIB_INSTALL_PREFIX}
    ARCHIVE
    DESTINATION ${PYLIB_INSTALL_PREFIX}
    RUNTIME
    DESTINATION ${PYLIB_INSTALL_PREFIX}
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${PYTHON_LIBRARY_NAME}> DESTINATION ${PYLIB_INSTALL_PREFIX} OPTIONAL)
endif()

install(CODE
    "file(TOUCH \"${PYLIB_INSTALL_PREFIX}/../__init__.py\")")

install(CODE
    "file(WRITE \"${PYLIB_INSTALL_PREFIX}/__init__.py\"
    \"from ${PYTHON_LIBRARY_NAME} import *\")")

add_subdirectory(usd)

add_subdirectory(render/mayaToHydra)
add_dependencies(mtoh hdMaya)
