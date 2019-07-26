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

#==============================================================================
# Modules and Definitions
#==============================================================================
list(APPEND CMAKE_MODULE_PATH
    ${PROJECT_SOURCE_DIR}/../cmake/defaults
    ${PROJECT_SOURCE_DIR}/../cmake/macros
    ${PROJECT_SOURCE_DIR}/../cmake/modules
)

#==============================================================================
# Compiler
#==============================================================================
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

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (APPLE)
   set(CMAKE_MACOSX_RPATH ON)
   set(OSX_ARCHITECTURES "x86_64")
   add_definitions(-DOSMac_ -DMAC_PLUGIN)
   add_definitions(-D_BOOL -DREQUIRE_IOSTREAM)
endif()

#==============================================================================
# Packages
#==============================================================================

find_package(Maya REQUIRED)
find_package(USD REQUIRED)
include(${USD_CONFIG_FILE})

if("${MAYA_DEVKIT_LOCATION}" STREQUAL "")
    set(CMAKE_WANT_UFE_BUILD OFF)
else()
    set(CMAKE_WANT_UFE_BUILD ON)
endif()

if(CMAKE_WANT_UFE_BUILD)
    find_package(UFE REQUIRED)
    add_definitions(-DWANT_UFE_BUILD)

    # UFE Group interface is available on 2.0+ and 0.2+ but not 1.x.
    if((NOT UFE_VERSION VERSION_LESS 2.0.0) OR
       ((UFE_VERSION VERSION_LESS 1.0.0) AND (NOT UFE_VERSION VERSION_LESS 0.2.0)))
        set(CMAKE_UFE_GROUP_INTERFACE_AVAILABLE ON)
        add_definitions(-DUFE_GROUP_INTERFACE_AVAILABLE)
    else()
        set(CMAKE_UFE_GROUP_INTERFACE_AVAILABLE OFF)
    endif()

    if(MSVC)
        # Silence this warning until we fix it in UFE.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251")
    endif()
endif()

#==============================================================================
# Generate namespace declaration header
#==============================================================================

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/base/mayaUsd.h.src ${CMAKE_CURRENT_BINARY_DIR}/mayaUsd/mayaUsd.h)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mayaUsd/mayaUsd.h
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/
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
	utils/blockSceneModificationContext.cpp
	utils/stageCache.cpp
    utils/query.cpp
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
	render/vp2RenderDelegate/render_param.cpp
	render/vp2RenderDelegate/instancer.cpp
	render/vp2RenderDelegate/draw_item.cpp
	render/vp2RenderDelegate/material.cpp
	render/vp2RenderDelegate/mesh.cpp
	render/vp2RenderDelegate/proxyRenderDelegate.cpp
	render/vp2RenderDelegate/render_delegate.cpp
	render/vp2RenderDelegate/sampler.cpp
	render/vp2RenderDelegate/texture.cpp
	render/vp2RenderDelegate/textureResource.cpp
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

if(CMAKE_WANT_UFE_BUILD)
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

    if(CMAKE_UFE_GROUP_INTERFACE_AVAILABLE)
        list(APPEND mayaUsd_src ufe/UsdUndoCreateGroupCommand.cpp)
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
	utils/blockSceneModificationContext.h
	utils/stageCache.h
    utils/query.h
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

if(CMAKE_WANT_UFE_BUILD)
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

    if(CMAKE_UFE_GROUP_INTERFACE_AVAILABLE)
        list(APPEND mayaUsdUfe_headers ufe/UsdUndoCreateGroupCommand.h)
    endif()
endif()

list(APPEND mayaUsdPxrUsdMayaGL_headers
	render/pxrUsdMayaGL/batchRenderer.h
	render/pxrUsdMayaGL/hdImagingShapeUI.h
	render/pxrUsdMayaGL/hdImagingShapeDrawOverride.h
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

add_library(${LIBRARY_NAME}
    SHARED
        ${mayaUsd_src}
)

target_compile_definitions(${LIBRARY_NAME}
    PRIVATE
        MAYAUSD_MACROS_EXPORT
        MAYAUSD_CORE_EXPORT
        MFB_PACKAGE_NAME="${LIBRARY_NAME}"
        MFB_ALT_PACKAGE_NAME="${LIBRARY_NAME}"
        MFB_PACKAGE_MODULE=mayaUsd
)

target_include_directories(
    ${LIBRARY_NAME}
    PUBLIC
    ${MAYA_INCLUDE_DIRS}
    ${UFE_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../base
	${PXR_INCLUDE_DIRS}
)

target_link_libraries(${LIBRARY_NAME}
    ${MAYA_Foundation_LIBRARY}
    ${MAYA_OpenMayaAnim_LIBRARY}
    ${MAYA_OpenMaya_LIBRARY}
    ${MAYA_OpenMayaRender_LIBRARY}
    ${MAYA_OpenMayaUI_LIBRARY}
    ${UFE_LIBRARY}
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

if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set_target_properties(${LIBRARY_NAME}
        PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "${PXR_USD_LOCATION}/lib;${PXR_USD_LOCATION}/lib64"
    )
else()
    set_target_properties(${LIBRARY_NAME}
        PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "${PXR_USD_LOCATION}/lib:${PXR_USD_LOCATION}/lib64"
    )
endif()

install(TARGETS ${LIBRARY_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${LIBRARY_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL)
endif()

set_property(GLOBAL PROPERTY GLOBAL_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

# install public headers
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

if(CMAKE_WANT_UFE_BUILD)
    install(FILES ${mayaUsdUfe_headers}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/ufe
    )
endif()

install(FILES ${mayaUsdPxrUsdMayaGL_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/render/pxrUsdMayaGL
)
 
add_subdirectory(usd)

add_subdirectory(render/mayaToHydra)
add_dependencies(mtoh hdmaya)