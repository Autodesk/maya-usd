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
project(mayaUsd)

add_library(${PROJECT_NAME} SHARED)

#------------------------------------------------------------------------------
# modules and definitions
#------------------------------------------------------------------------------
if(NOT DEFINED ENV{BOOST_ROOT})
    set(ENV{BOOST_ROOT} ${PXR_USD_LOCATION})
endif()
if(MAYAUSD_DEFINE_BOOST_DEBUG_PYTHON_FLAG)
    set(Boost_USE_DEBUG_PYTHON ON)
endif()

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        PXR_PLUGINPATH_NAME=${PXR_OVERRIDE_PLUGINPATH_NAME}
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
        $<$<BOOL:${UFE_FOUND}>:WANT_UFE_BUILD>
    PRIVATE
        MAYAUSD_MACROS_EXPORT
        MAYAUSD_CORE_EXPORT
        MFB_PACKAGE_NAME="${PROJECT_NAME}"
        MFB_ALT_PACKAGE_NAME="${PROJECT_NAME}"
        MFB_PACKAGE_MODULE="${PROJECT_NAME}"
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<BOOL:${IS_LINUX}>:GL_GLEXT_PROTOTYPES>
        $<$<BOOL:${IS_LINUX}>:GLX_GLXEXT_PROTOTYPES>
        $<$<BOOL:${CMAKE_WANT_MATERIALX_BUILD}>:WANT_MATERIALX_BUILD>
        $<$<BOOL:${Qt5_FOUND}>:WANT_QT_BUILD>
        $<$<BOOL:${BUILD_HDMAYA}>:BUILD_HDMAYA>

        # this flag is needed when building maya-usd in Maya
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
)

if(DEFINED UFE_PREVIEW_VERSION_NUM)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
            UFE_PREVIEW_VERSION_NUM=${UFE_PREVIEW_VERSION_NUM}
    )
endif()

message(STATUS "MAYA_HAS_DEFAULT_MATERIAL_API is ${MAYA_HAS_DEFAULT_MATERIAL_API}")
if (MAYA_HAS_DEFAULT_MATERIAL_API)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            HAS_DEFAULT_MATERIAL_SUPPORT_API=1
    )
endif()

message(STATUS "MAYA_NEW_POINT_SNAPPING_SUPPORT is ${MAYA_NEW_POINT_SNAPPING_SUPPORT}")
if (MAYA_NEW_POINT_SNAPPING_SUPPORT)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        MAYA_NEW_POINT_SNAPPING_SUPPORT=1
    )
endif()

message(STATUS "MAYA_HAS_CRASH_DETECTION is ${MAYA_HAS_CRASH_DETECTION}")
if (MAYA_HAS_CRASH_DETECTION)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        MAYA_HAS_CRASH_DETECTION=1
    )
endif()

message(STATUS "MAYA_CURRENT_UFE_CAMERA_SUPPORT is ${MAYA_CURRENT_UFE_CAMERA_SUPPORT}")
if (MAYA_CURRENT_UFE_CAMERA_SUPPORT)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    MAYA_CURRENT_UFE_CAMERA_SUPPORT=1
    )
endif()

message(STATUS "MAYA_MRENDERITEM_UFE_IDENTIFIER_SUPPORT is ${MAYA_MRENDERITEM_UFE_IDENTIFIER_SUPPORT}")
if (MAYA_MRENDERITEM_UFE_IDENTIFIER_SUPPORT)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    MAYA_MRENDERITEM_UFE_IDENTIFIER_SUPPORT=1
    )
endif()

message(STATUS "MAYA_UPDATE_UFE_IDENTIFIER_SUPPORT is ${MAYA_UPDATE_UFE_IDENTIFIER_SUPPORT}")
if (MAYA_UPDATE_UFE_IDENTIFIER_SUPPORT)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    MAYA_UPDATE_UFE_IDENTIFIER_SUPPORT=1
    )
endif()

if (MAYA_LIGHTAPI_VERSION EQUAL 2)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            MAYA_LIGHTAPI_VERSION_2=2
    )
endif()

# Some of the UFE classes are exporting STL classes which causes this warning.
if(UFE_FOUND AND MSVC)
    target_compile_options(${PROJECT_NAME}
        PUBLIC
            /wd4251
    )
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        MAYA_USD_VERSION=${USD_VERSION}
        MAYAUSD_VERSION=${MAYAUSD_VERSION}
        MAYA_PY_VERSION=${MAYA_PY_VERSION}
)

if(CMAKE_WANT_MATERIALX_BUILD)
# Path where MaterialX was found. Allows loading the required libraries.
string(REPLACE "\\" "/" Forward_MaterialX_DIR ${MaterialX_DIR})
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        MaterialX_DIR="${Forward_MaterialX_DIR}"
)
endif()

mayaUsd_compile_config(${PROJECT_NAME})

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} 
    PUBLIC
        ${MAYA_INCLUDE_DIRS}
        ${PXR_INCLUDE_DIRS}
        $<$<BOOL:${UFE_FOUND}>:${UFE_INCLUDE_DIR}>
        ${CMAKE_BINARY_DIR}/include
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ar
        gf
        hd
        hdx
        js
        kind
        plug
        sdf
        tf
        trace
        usd
        usdGeom
        usdImaging
        usdImagingGL
        usdLux
        usdRender
        usdRi
        usdShade
        usdSkel
        usdUtils
        vt
        $<$<BOOL:${UFE_FOUND}>:${UFE_LIBRARY}>
        ${MAYA_LIBRARIES}
        mayaUsdUtils
    PRIVATE
        ghc_filesystem
        $<$<BOOL:${CMAKE_WANT_MATERIALX_BUILD}>:hdMtlx>
        $<$<BOOL:${CMAKE_WANT_MATERIALX_BUILD}>:MaterialXCore>
        $<$<BOOL:${CMAKE_WANT_MATERIALX_BUILD}>:MaterialXFormat>
        $<$<BOOL:${CMAKE_WANT_MATERIALX_BUILD}>:MaterialXRender>
        $<$<BOOL:${CMAKE_WANT_MATERIALX_BUILD}>:MaterialXGenOgsXml>
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

#install top level plugInfo.json that includes the configured plugInfo.json
install(CODE
    "file(WRITE \"${CMAKE_CURRENT_BINARY_DIR}/lib/usd/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib/usd/plugInfo.json
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd
)

# -----------------------------------------------------------------------------
# subdirectories
# -----------------------------------------------------------------------------
add_subdirectory(base)
add_subdirectory(commands)
add_subdirectory(fileio)
add_subdirectory(listeners)
add_subdirectory(nodes)
add_subdirectory(python)
add_subdirectory(render)
add_subdirectory(resources)
if(UFE_FOUND)
    add_subdirectory(ufe)
endif()
add_subdirectory(undo)
add_subdirectory(utils)

if(BUILD_HDMAYA)
    add_subdirectory(render/mayaToHydra)
endif()
