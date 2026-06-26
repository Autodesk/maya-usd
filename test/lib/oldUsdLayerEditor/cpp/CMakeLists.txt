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

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# *.cpp test sources and testUtils.h live in the new editor's test/cpp dir
# and are shared between old and new test suites.
set(NEW_LE_TEST_CPP
    ${CMAKE_CURRENT_SOURCE_DIR}/../../usdLayerEditor/cpp)

# Sources
set(OLD_LE_OWN_SOURCES
    stubCommandHook.cpp
    stubSessionState.cpp
    testFixture.cpp
    pluginMain.cpp
)

# Shared test logic compiled directly from the new editor's test dir. Include
# path order (old test dir first) makes `#include "testFixture.h"` resolve to
# the old fixture, which selects the old-editor code paths.
set(OLD_LE_SHARED_LOGIC_SOURCES
    ${NEW_LE_TEST_CPP}/testButtons.cpp
    ${NEW_LE_TEST_CPP}/testComponentSaveWidget.cpp
    ${NEW_LE_TEST_CPP}/testContextMenu.cpp
    ${NEW_LE_TEST_CPP}/testEFMode.cpp
    ${NEW_LE_TEST_CPP}/testLayerContentsWidget.cpp
    ${NEW_LE_TEST_CPP}/testLayerEditorWidget.cpp
    ${NEW_LE_TEST_CPP}/testLayerEditorWindow.cpp
    ${NEW_LE_TEST_CPP}/testLayerLogicUtils.cpp
    ${NEW_LE_TEST_CPP}/testLayerLocking.cpp
    ${NEW_LE_TEST_CPP}/testLayerMuting.cpp
    ${NEW_LE_TEST_CPP}/testLayerTreeItem.cpp
    ${NEW_LE_TEST_CPP}/testLayerTreeItemDelegate.cpp
    ${NEW_LE_TEST_CPP}/testLayerTreeModel.cpp
    ${NEW_LE_TEST_CPP}/testLayerTreeView.cpp
    ${NEW_LE_TEST_CPP}/testLayerTreeViewMouse.cpp
    ${NEW_LE_TEST_CPP}/testLoadLayersDialog.cpp
    ${NEW_LE_TEST_CPP}/testMenusAndStage.cpp
    ${NEW_LE_TEST_CPP}/testPathChecker.cpp
    ${NEW_LE_TEST_CPP}/testReorder.cpp
    ${NEW_LE_TEST_CPP}/testSaveLayersDialog.cpp
    ${NEW_LE_TEST_CPP}/testSessionState.cpp
    ${NEW_LE_TEST_CPP}/testSharedStage.cpp
    ${NEW_LE_TEST_CPP}/testStageSelectorWidget.cpp
    ${NEW_LE_TEST_CPP}/testUsdSyntaxHighlighter.cpp
)

# Legacy widget sources compiled directly into this test DLL. This gives the
# test a self-contained copy of the old editor, avoiding ODR conflicts with
# usdLayerEditor, which exports the new editor's versions of the same classes.
set(LEGACY_SOURCES
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/mayaQtUtils.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/componentSaveWidget.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/dirtyLayersCountBadge.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/generatedIconButton.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/layerContentsWidget.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/layerEditorWidget.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/layerTreeItem.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/layerTreeItemDelegate.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/layerTreeModel.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/layerTreeView.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/loadLayersDialog.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/pathChecker.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/qtUtils.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/resources.qrc
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/saveLayersDialog.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/sessionState.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/stageSelectorWidget.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/stringResources.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/usdSyntaxHighlighter.cpp
    ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor/warningDialogs.cpp
)

# The legacy layerEditorWidget.cpp above opens the edit-forward dialog, so its
# definition must be compiled in when edit-forward is available; exercised by
# the shared testEFMode.cpp cases.
if(AdskUsdEditForward_FOUND)
    list(APPEND LEGACY_SOURCES
        ${CMAKE_SOURCE_DIR}/lib/usd/ui/editForward/editForwardDialog.cpp)
endif()

# Target
set(TARGET_NAME mayaUsdOldLayerEditorTests)

add_library(${TARGET_NAME} SHARED
    ${OLD_LE_OWN_SOURCES}
    ${OLD_LE_SHARED_LOGIC_SOURCES}
    ${LEGACY_SOURCES}
)

if(IS_WINDOWS)
    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".mll")
elseif(IS_MACOSX)
    set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "" SUFFIX ".bundle")
endif()

mayaUsd_compile_config(${TARGET_NAME})

target_compile_definitions(${TARGET_NAME}
    PRIVATE
        MAYAUSD_UI_EXPORT
        # Marks the shared logic-header tests as compiling against the old editor,
        # so old-editor-only parity gaps can be skipped. Only this target defines it.
        MAYAUSD_OLD_LAYER_EDITOR
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

# Include path order is critical:
#   1. OLD editor test dir FIRST — shim headers shadow new editor versions.
#   2. NEW editor test dir SECOND — shared test .cpp sources and testUtils.h.
#   3. OLD editor lib LAST — abstractCommandHook.h etc. must shadow the new lib versions.
target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NEW_LE_TEST_CPP}
        ${CMAKE_SOURCE_DIR}/lib/usd/ui/layerEditor
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        mayaUsd
        GTest::GTest
        Qt::Core Qt::Gui Qt::Widgets
        sdf tf usd
        ${MAYA_LIBRARIES}
        $<$<BOOL:${AdskUsdEditForward_FOUND}>:${ADSK_USD_EDIT_FORWARD_UI_LIBRARY}>
)

install(TARGETS ${TARGET_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/maya
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/maya
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}>
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/maya OPTIONAL)
endif()
