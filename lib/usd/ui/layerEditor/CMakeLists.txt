
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

# When using the shared usd-layer-editor component, only the Maya-specific
# wiring files build here. All shared sources are provided by UsdLayerEditorLib.
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
)
target_link_libraries(${PROJECT_NAME} PRIVATE UsdLayerEditorLib)
target_compile_definitions(${PROJECT_NAME} PRIVATE MAYAUSD_USE_SHARED_LAYER_EDITOR=1)

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

