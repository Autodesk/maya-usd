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

# This CMakeLists is invoked from lib/CMakeLists.txt.
# Qt, UFE, USD and usdUfe are already found/defined by the parent build —
# do not call find_package here.

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(UsdLayerEditorLib SHARED
    abstractCommandHook.h
    abstractLayerEditorWindow.h
    batchSaveLayersUIDelegate.cpp
    batchSaveLayersUIDelegate.h
    componentSaveWidget.cpp
    componentSaveWidget.h
    customLayerData.cpp
    customLayerData.h
    dirtyLayersCountBadge.cpp
    dirtyLayersCountBadge.h
    generatedIconButton.cpp
    generatedIconButton.h
    layerContentsWidget.cpp
    layerContentsWidget.h
    layerEditorAPI.h
    LayerEditorCommands.h
    layerEditorCommands.cpp
    layerEditorDCCFunctions.cpp
    layerEditorDCCFunctions.h
    layerEditorWidget.cpp
    layerEditorWidget.h
    layerEditorWidgetManager.cpp
    layerEditorWidgetManager.h
    layerEditorWindow.cpp
    layerEditorWindow.h
    layerLocking.cpp
    layerLocking.h
    layerMuting.cpp
    layerMuting.h
    layers.cpp
    layers.h
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
    tokens.cpp
    tokens.h
    ufeCommandHook.cpp
    ufeCommandHook.h
    usdSyntaxHighlighter.cpp
    usdSyntaxHighlighter.h
    utilFileSystem.cpp
    utilFileSystem.h
    utilQT.cpp
    utilQT.h
    utilSerialization.cpp
    utilSerialization.h
    utilString.h
    utilUI.cpp
    utilUI.h
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

target_compile_definitions(UsdLayerEditorLib
    PUBLIC
        $<$<BOOL:${AdskUsdEditForward_FOUND}>:WANT_ADSK_USD_EDIT_FORWARD_BUILD>
    PRIVATE
        LAYEREDITOR_EXPORTS
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

mayaUsd_compile_config(UsdLayerEditorLib)

target_include_directories(UsdLayerEditorLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${UFE_INCLUDE_DIR}
        $<$<BOOL:${AdskUsdEditForward_FOUND}>:${ADSK_USD_EDIT_FORWARD_INCLUDE_DIR}>
)

target_link_libraries(UsdLayerEditorLib
    PUBLIC
        usdUfe
        ${UFE_LIBRARY}
        sdf
        tf
        usd
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

# run-time search paths (mirrors usdUfe)
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "lib")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    mayaUsd_install_rpath(rpath UsdLayerEditorLib)
endif()

install(TARGETS UsdLayerEditorLib
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

# Install the USD syntax highlighting configuration file. The cpp resolves the path
# at runtime via MAYAUSD_LIB_LOCATION + /syntaxHighlight/usdSyntaxConfig.json (or via
# the MAYAUSD_USD_SYNTAX_HIGHLIGHTING_CONFIG env var for user overrides).
install(FILES "usdSyntaxConfig.json"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/syntaxHighlight"
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:UsdLayerEditorLib>
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL
    )
endif()
