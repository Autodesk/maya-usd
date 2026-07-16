
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

# lib/usd/ui/layerEditor/ contains pre-migration copies of headers that share
# names with the shared layer editor (sessionState.h, abstractCommandHook.h,
# etc.). Qt AUTOMOC injects this directory into the include search path, which
# would shadow the shared versions. Declare the shared paths explicitly first
# so they take precedence.
# Note: this workaround will be removed when the old layer editor is completely deleted.
target_include_directories(${PROJECT_NAME} BEFORE PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/usdLayerEditor/ui
    ${CMAKE_SOURCE_DIR}/lib/usdLayerEditor/lib
)

target_sources(${PROJECT_NAME}
    PRIVATE
        batchSaveLayersUIDelegate.cpp
        batchSaveLayersUIDelegate.h
        mayaCommandHook.cpp
        mayaCommandHook.h
        mayaLayerEditorWindow.cpp
        mayaLayerEditorWindow.h
        mayaSessionState.cpp
        mayaSessionState.h
        mayaQtUtils.cpp
        mayaQtUtils.h
        mayaLayerEditorUi.h
)
target_link_libraries(${PROJECT_NAME} PRIVATE usdLayerEditorUi)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    MAYAUSD_USE_SHARED_LAYER_EDITOR=1
    # MSVC searches "currently open include files" directories (step 2) before
    # /I paths (step 3) for quoted includes. When lib/usdLayerEditor/ui/sessionState.h
    # does #include "abstractCommandHook.h" while lib/usd/ui/layerEditor/mayaSessionState.h
    # is open in the stack, MSVC finds the old header via step 2. Pre-defining the
    # old guard silently skips its body; the new version (found first via BEFORE /I paths
    # for angle-bracket includes) provides the full AbstractCommandHook definition.
    # Note: this workaround will be removed when the old layer editor is completely deleted.
    ABSTRACTCOMMANDHOOK_H
)

# -----------------------------------------------------------------------------
# MSVC fix: avoid ambiguous qfloat16 arithmetic operators pulled by Qt headers
# Only needed for <= VS2017 
# -----------------------------------------------------------------------------
if (MSVC AND MSVC_VERSION LESS 1920)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            QT_NO_FLOAT16_OPERATORS
    )
endif()

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
set(HEADERS
    batchSaveLayersUIDelegate.h
    mayaLayerEditorUi.h
)

mayaUsd_promoteHeaderList( 
    HEADERS
        ${HEADERS}
    BASEDIR
        ${PROJECT_NAME}/ui
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/ui
)

# Install the USD syntax highlighting configuration file.
install(FILES "usdSyntaxConfig.json"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/syntaxHighlight"
)

