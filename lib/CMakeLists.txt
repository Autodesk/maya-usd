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

if(IS_MACOSX)
    list(APPEND compile_definitions_list OSMac_)
endif()

target_compile_definitions(${LIBRARY_NAME} PRIVATE
    ${compile_definitions_list}
)

set(include_directories_list
    ${MAYA_INCLUDE_DIRS}
    ${PXR_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/include
)

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

install(FILES ${mayaUsdShaderFragments_xmls}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd/mayaUsd_ShaderFragments/resources
)

#install top level plugInfo.json that includes the configured plugInfo.json
install(CODE
    "file(WRITE \"${CMAKE_INSTALL_PREFIX}/lib/usd/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)

if(IS_MACOSX)
    set(_macDef OSMac_)
endif()
