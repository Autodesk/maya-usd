
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

target_sources(${PROJECT_NAME}
    PRIVATE
        batchSaveLayersUIDelegate.cpp
        dirtyLayersCountBadge.cpp
        generatedIconButton.cpp
        layerEditorWidget.cpp
        layerEditorWidget.h
        layerTreeItem.cpp
        layerTreeItem.h
        layerTreeItemDelegate.cpp
        layerTreeItemDelegate.h
        layerTreeModel.cpp
        layerTreeModel.h
        layerTreeView.cpp
        layerTreeView.h
        loadLayersDialog.cpp
        loadLayersDialog.h
        mayaCommandHook.cpp
        mayaCommandHook.h
        mayaLayerEditorWindow.cpp
        mayaLayerEditorWindow.h
        mayaSessionState.cpp
        mayaSessionState.h
        mayaQtUtils.cpp
        mayaQtUtils.h
        pathChecker.cpp
        pathChecker.h
        qtUtils.cpp
        qtUtils.h
        resources.qrc
        saveLayersDialog.cpp
        saveLayersDialog.h
        sessionState.cpp
        sessionState.h
        stageSelectorWidget.cpp
        stageSelectorWidget.h
        stringResources.cpp
        stringResources.h
        warningDialogs.cpp
        warningDialogs.h
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ghc_filesystem
)

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
