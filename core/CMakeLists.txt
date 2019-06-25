#-
#*****************************************************************************
# Copyright 2019 Autodesk, Inc. All rights reserved.
#
# These coded instructions, statements, and computer programs contain
# unpublished proprietary information written by Autodesk, Inc. and are
# protected by Federal copyright law. They may not be disclosed to third
# parties or copied or duplicated in any form, in whole or in part, without
# the prior written consent of Autodesk, Inc.
#*****************************************************************************
#+

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
    ${PROJECT_SOURCE_DIR}/../cmake/modules
)

#==============================================================================
# Compiler
#==============================================================================
# CXXDefaults will set a variety of variables for the project.
# Consume them here. This is an effort to keep the most common
# build files readable.
include(CXXDefaults)
add_definitions(${_PXR_CXX_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${_PXR_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS
        -msse3
        "${CMAKE_CXX_FLAGS} ${_PXR_CXX_FLAGS}"
    )
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if (WIN32)
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

if (APPLE)
   set(CMAKE_MACOSX_RPATH ON)
endif()

#==============================================================================
# Packages
#==============================================================================
find_package(Maya REQUIRED)
find_package(USD REQUIRED)
include(${USD_CONFIG_FILE})

#==============================================================================
# Generate namespace declaration header
#==============================================================================

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/base/mayaUsd.h.src ${CMAKE_CURRENT_BINARY_DIR}/mayaUsdCore/mayaUsd.h)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mayaUsdCore/mayaUsd.h
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsdCore/
)

#==============================================================================
# Library
#==============================================================================

set(LIBRARY_NAME mayaUsdCore)

list(APPEND mayaUsdCore_src
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
	#
	fileio/chaser/chaser.cpp
	fileio/chaser/chaserRegistry.cpp
	#
	fileio/jobs/jobArgs.cpp
	fileio/jobs/readJob.cpp
	fileio/jobs/readJob_ImportWithProxies.cpp
	fileio/jobs/writeJob.cpp
	fileio/jobs/modelKindProcessor.cpp
	#
	fileio/translators/translatorUtil.cpp
	fileio/translators/translatorXformable.cpp
	fileio/translators/translatorXformable_decompose.cpp
	fileio/translators/translatorPrim.cpp
	fileio/translators/translatorGPrim.cpp
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
	utils/util.cpp
	utils/blockSceneModificationContext.cpp
	utils/stageCache.cpp
	#
	nodes/pointBasedDeformerNode.cpp
	nodes/hdImagingShape.cpp
	nodes/usdPrimProvider.cpp
	nodes/proxyShapeBase.cpp
	nodes/stageData.cpp
	nodes/stageNode.cpp
	#
	listeners/stageNoticeListener.cpp
	listeners/notice.cpp
	#
	renderers/vp2RenderDelegate/render_param.cpp
	renderers/vp2RenderDelegate/instancer.cpp
	renderers/vp2RenderDelegate/draw_item.cpp
	renderers/vp2RenderDelegate/material.cpp
	renderers/vp2RenderDelegate/mesh.cpp
	renderers/vp2RenderDelegate/proxyRenderDelegate.cpp
	renderers/vp2RenderDelegate/render_delegate.cpp
	renderers/vp2RenderDelegate/sampler.cpp
	renderers/vp2RenderDelegate/texture.cpp
	renderers/vp2RenderDelegate/textureResource.cpp
)

list(APPEND mayaUsdCoreBase_headers
    base/api.h
    base/debugCodes.h
)

list(APPEND mayaUsdCoreFileio_headers
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
)

list(APPEND mayaUsdCoreChaser_headers
	fileio/chaser/chaser.h
	fileio/chaser/chaserRegistry.h
)

list(APPEND mayaUsdCoreJobs_headers
	fileio/jobs/jobArgs.h
	fileio/jobs/readJob.h
	fileio/jobs/writeJob.h
	fileio/jobs/modelKindProcessor.h
)

list(APPEND mayaUsdCoreShading_headers
	fileio/shading/shadingModeExporterContext.h
	fileio/shading/shadingModeExporter.h
	fileio/shading/shadingModeImporter.h
	fileio/shading/shadingModeRegistry.h
)

list(APPEND mayaUsdCoreUtilsIO_headers
	fileio/utils/adaptor.h
	fileio/utils/readUtil.h
	fileio/utils/userTaggedAttribute.h
	fileio/utils/writeUtil.h
	fileio/utils/shadingUtil.h
	fileio/utils/meshUtil.h
	fileio/utils/roundTripUtil.h
	fileio/utils/xformStack.h
)

list(APPEND mayaUsdCoreTranslators_headers
	fileio/translators/translatorUtil.h
	fileio/translators/translatorXformable.h
	fileio/translators/translatorPrim.h
	fileio/translators/translatorGPrim.h
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

list(APPEND mayaUsdCoreUtils_headers
	utils/colorSpace.h
	utils/util.h
	utils/blockSceneModificationContext.h
	utils/stageCache.h
)

list(APPEND mayaUsdCoreNodes_headers
	nodes/pointBasedDeformerNode.h
	nodes/hdImagingShape.h
	nodes/usdPrimProvider.h
	nodes/proxyShapeBase.h
	nodes/stageData.h
	nodes/stageNode.h
)

list(APPEND mayaUsdCoreListeners_headers
	listeners/stageNoticeListener.h
	listeners/notice.h
)

list(APPEND mayaUsdCoreVP2RenderDelegate_headers
	renderers/vp2RenderDelegate/proxyRenderDelegate.h
)

add_library(${LIBRARY_NAME}
    SHARED
        ${mayaUsdCore_src}
)

target_compile_definitions(${LIBRARY_NAME}
    PRIVATE
        MAYAUSD_MACROS_EXPORT
        MAYAUSD_CORE_EXPORT
        MFB_PACKAGE_NAME="${LIBRARY_NAME}"
        MFB_ALT_PACKAGE_NAME="${LIBRARY_NAME}"
        MFB_PACKAGE_MODULE=mayaUsdCore
)

target_include_directories(
    ${LIBRARY_NAME}
    PUBLIC
    ${MAYA_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../base
    # MayaUsd.h is generated in the build area
    ${CMAKE_CURRENT_BINARY_DIR}/../base
	${PXR_INCLUDE_DIRS}
)

target_link_libraries(${LIBRARY_NAME}
    ${MAYA_Foundation_LIBRARY}
    ${MAYA_OpenMayaAnim_LIBRARY}
    ${MAYA_OpenMaya_LIBRARY}
    ${MAYA_OpenMayaRender_LIBRARY}
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
    usdLux
    usdRi
    usdShade
    usdSkel
    usdUtils
    vt
	
)

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
install(FILES ${mayaUsdCoreBase_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsdCore/base/
)

install(FILES ${mayaUsdCoreFileio_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsdCore/fileio/
)

install(FILES ${mayaUsdCoreChaser_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsdCore/fileio/chaser/
)

install(FILES ${mayaUsdCoreJobs_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsdCore/fileio/jobs/
)

install(FILES ${mayaUsdCoreUtilsIO_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsdCore/fileio/utils/
)

install(FILES ${mayaUsdCoreShading_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsdCore/fileio/shading/
)

install(FILES ${mayaUsdCoreTranslators_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsdCore/fileio/translators/
)

install(FILES ${mayaUsdCoreUtils_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsdCore/utils/
)

install(FILES ${mayaUsdCoreNodes_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsdCore/nodes/
)

install(FILES ${mayaUsdCoreListeners_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsdCore/listeners/
)

install(FILES ${mayaUsdCoreVP2RenderDelegate_headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsdCore/renderers/vp2RenderDelegate
)

add_subdirectory(schemas)
