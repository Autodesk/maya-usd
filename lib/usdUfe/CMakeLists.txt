#
# Copyright 2023 Autodesk
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
project(usdUfe)

include(usdUfe_version.info)
set(USDUFE_VERSION "${USDUFE_MAJOR_VERSION}.${USDUFE_MINOR_VERSION}.${USDUFE_PATCH_LEVEL}")

add_library(${PROJECT_NAME} SHARED)

# -----------------------------------------------------------------------------
# Compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
        $<$<BOOL:${MAYA_ENABLE_NEW_PRIM_DELETE}>:MAYA_ENABLE_NEW_PRIM_DELETE>
    PRIVATE
        USDUFE_EXPORT
        MFB_PACKAGE_NAME="${PROJECT_NAME}"
        MFB_ALT_PACKAGE_NAME="${PROJECT_NAME}"
        MFB_PACKAGE_MODULE="${PROJECT_NAME}"
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        $<$<BOOL:${IS_LINUX}>:LINUX>

        # this flag is needed when building for Maya
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
)

message(STATUS "USD_HAS_COLOR4_SDR_SUPPORT is ${USD_HAS_COLOR4_SDR_SUPPORT}")
if (USD_HAS_COLOR4_SDR_SUPPORT)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            USD_HAS_COLOR4_SDR_SUPPORT=1
    )
endif()

message(STATUS "USD_HAS_NAMESPACE_EDIT is ${USD_HAS_NAMESPACE_EDIT}")
if (USD_HAS_NAMESPACE_EDIT)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        USD_HAS_NAMESPACE_EDIT=1
    )
endif()

message(STATUS "MAYA_HAS_DISPLAY_LAYER_API is ${MAYA_HAS_DISPLAY_LAYER_API}")
if (MAYA_HAS_DISPLAY_LAYER_API)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            MAYA_HAS_DISPLAY_LAYER_API=1
    )
endif()

message(STATUS "UFE_SCENEITEM_HAS_METADATA is ${UFE_SCENEITEM_HAS_METADATA}")
if (UFE_SCENEITEM_HAS_METADATA)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_SCENEITEM_HAS_METADATA=1
    )
endif()

if (UFE_HAS_DISPLAY_NAME)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_HAS_DISPLAY_NAME=1
    )
endif()


if (UFE_DEFAULT_VALUE_SUPPORT)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_DEFAULT_VALUE_SUPPORT=1
    )
endif()

if (UFE_HAS_UNSIGNED_INT)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_HAS_UNSIGNED_INT=1
    )
endif()

# Some of the UFE classes are exporting STL classes which causes this warning.
if(MSVC)
    target_compile_options(${PROJECT_NAME}
        PUBLIC
            /wd4251
    )
endif()

mayaUsd_compile_config(${PROJECT_NAME})

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} 
    PUBLIC
        ${PXR_INCLUDE_DIRS}
        ${UFE_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}/include
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        gf
        sdf
        tf
        usd
        usdGeom
        usdImaging
        usdShade
        usdUtils
        vt
        ${UFE_LIBRARY}
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

# -----------------------------------------------------------------------------
# Subdirectories
# -----------------------------------------------------------------------------
add_subdirectory(base)
add_subdirectory(python)
add_subdirectory(resources)
add_subdirectory(ufe)
add_subdirectory(undo)
add_subdirectory(utils)
