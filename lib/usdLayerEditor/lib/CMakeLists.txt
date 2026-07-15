#
# Copyright 2026 Autodesk
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

# ============================================================
# usdLayerEditorLib — Qt-free commands, hooks and logic
# ============================================================
add_library(usdLayerEditorLib SHARED
    abstractCommandHook.h
    customLayerData.cpp
    customLayerData.h
    layerEditorAPI.h
    LayerEditorCommands.h
    layerEditorCommands.cpp
    layerEditorDCCFunctions.cpp
    layerEditorDCCFunctions.h
    layerLocking.cpp
    layerLocking.h
    layerMuting.cpp
    layerMuting.h
    layers.cpp
    layers.h
    tokens.cpp
    tokens.h
    utilFileSystem.cpp
    utilFileSystem.h
    utilSerialization.cpp
    utilSerialization.h
    utilString.h
)

target_compile_definitions(usdLayerEditorLib
    PRIVATE
        LAYEREDITOR_EXPORTS
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

mayaUsd_compile_config(usdLayerEditorLib)

target_include_directories(usdLayerEditorLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${UFE_INCLUDE_DIR}
)

target_link_libraries(usdLayerEditorLib
    PUBLIC
        usdUfe
        ${UFE_LIBRARY}
        sdf
        tf
        usd
)

if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "lib")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    mayaUsd_install_rpath(rpath usdLayerEditorLib)
endif()

install(TARGETS usdLayerEditorLib
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:usdLayerEditorLib>
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL
    )
endif()

# ============================================================
# usdLayerEditorUi — Qt widget layer
# ============================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(usdLayerEditorUi SHARED
    abstractLayerEditorWindow.h
    batchSaveLayersUIDelegate.cpp
    batchSaveLayersUIDelegate.h
    componentSaveWidget.cpp
    componentSaveWidget.h
    dirtyLayersCountBadge.cpp
    dirtyLayersCountBadge.h
    generatedIconButton.cpp
    generatedIconButton.h
    layerContentsWidget.cpp
    layerContentsWidget.h
    layerEditorWidget.cpp
    layerEditorWidget.h
    layerEditorWidgetManager.cpp
    layerEditorWidgetManager.h
    layerEditorWindow.cpp
    layerEditorWindow.h
    layerTreeItem.cpp
    layerTreeItem.h
    layerTreeItemDelegate.cpp
    layerTreeItemDelegate.h
    layerTreeModel.cpp
    layerTreeModel.h
    layerTreeView.cpp
    layerTreeView.h
    layerTreeViewStyle.h
    loadLayersDialog.cpp
    loadLayersDialog.h
    pathChecker.cpp
    pathChecker.h
    resources.qrc
    saveLayersDialog.cpp
    saveLayersDialog.h
    sessionState.cpp
    sessionState.h
    stageSelectorWidget.cpp
    stageSelectorWidget.h
    stringResources.cpp
    stringResources.h
    ufeCommandHook.cpp
    ufeCommandHook.h
    usdSyntaxHighlighter.cpp
    usdSyntaxHighlighter.h
    utilQT.cpp
    utilQT.h
    warningDialogs.cpp
    warningDialogs.h
)

# QT_NO_KEYWORDS: match mayaUsdUI convention (forbids `signals`/`slots`/`emit` macros)
get_target_property(qt_core_aliased Qt::Core ALIASED_TARGET)
if (qt_core_aliased)
    set_target_properties(${qt_core_aliased} PROPERTIES INTERFACE_COMPILE_DEFINITIONS QT_NO_KEYWORDS)
else()
    set_target_properties(Qt::Core PROPERTIES INTERFACE_COMPILE_DEFINITIONS QT_NO_KEYWORDS)
endif()

target_compile_definitions(usdLayerEditorUi
    PRIVATE
        LAYEREDITOR_UI_EXPORTS
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

mayaUsd_compile_config(usdLayerEditorUi)

target_include_directories(usdLayerEditorUi
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${UFE_INCLUDE_DIR}
)

target_link_libraries(usdLayerEditorUi
    PUBLIC
        usdLayerEditorLib
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "lib")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    mayaUsd_install_rpath(rpath usdLayerEditorUi)
endif()

install(TARGETS usdLayerEditorUi
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install(FILES "usdSyntaxConfig.json"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/syntaxHighlight"
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:usdLayerEditorUi>
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL
    )
endif()
