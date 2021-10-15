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
set(TARGET_NAME mayaUsdPlugin)

add_library(${TARGET_NAME} MODULE)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
        plugin.cpp
        adskImportCommand.cpp
        adskExportCommand.cpp
        adskListJobContextsCommand.cpp
        adskListShadingModesCommand.cpp
        adskStageLoadUnloadCommands.cpp
        importTranslator.cpp
        exportTranslator.cpp
        ProxyShape.cpp
)

# -----------------------------------------------------------------------------
# compile configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME}
    PRIVATE
        MAYAUSD_PLUGIN_EXPORT
        $<$<BOOL:${Qt5_FOUND}>:WANT_QT_BUILD>
)

if(DEFINED MAYAUSD_VERSION)
    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            MAYAUSD_VERSION=${MAYAUSD_VERSION}
    )
endif()

mayaUsd_compile_config(${TARGET_NAME})

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME}
    PRIVATE
        $<$<BOOL:${UFE_FOUND}>:${UFE_INCLUDE_DIR}>
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PRIVATE
        $<$<BOOL:${UFE_FOUND}>:${UFE_LIBRARY}>
        basePxrUsdPreviewSurface
)

if (${Qt5_FOUND})
    target_link_libraries(${TARGET_NAME}
        PRIVATE
            mayaUsdUI
    )
else()
    target_link_libraries(${TARGET_NAME}
        PRIVATE
            mayaUsd
    )
endif()

# -----------------------------------------------------------------------------
# properties
# -----------------------------------------------------------------------------
maya_set_plugin_properties(${TARGET_NAME})

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX) 
    mayaUsd_init_rpath(rpath "plugin")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if(IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(TARGETS
    ${TARGET_NAME}
    LIBRARY
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
    RUNTIME
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> DESTINATION ${INSTALL_DIR_SUFFIX}/plugin OPTIONAL)
endif()
