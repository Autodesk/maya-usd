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

# *Logic.h headers and testUtils.h live in the new editor's test/cpp dir and are
# shared between old and new test suites.
set(NEW_LE_TEST_CPP
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../test/lib/usdLayerEditor/cpp)

# layerEditorDCCFunctions.h lives in the shared lib; the *Logic.h headers use
# ScopedLayerEditorDCCFunctions which includes it transitively.
set(NEW_LE_LIB
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../lib/usdLayerEditor/lib)

# ── Sources ────────────────────────────────────────────────────────────────────
set(OLD_LE_OWN_SOURCES
    stubCommandHook.cpp
    stubSessionState.cpp
    testFixture.cpp
    pluginMain.cpp
    testButtons.cpp
    testComponentSaveWidget.cpp
    testContextMenu.cpp
    testEFMode.cpp
    testLayerContentsWidget.cpp
    testLayerEditorWidget.cpp
    testLayerEditorWindow.cpp
    testLayerLocking.cpp
    testLayerMuting.cpp
    testLayerTreeItem.cpp
    testLayerTreeItemDelegate.cpp
    testLayerTreeModel.cpp
    testLayerTreeView.cpp
    testLayerTreeViewMouse.cpp
    testLoadLayersDialog.cpp
    testMenusAndStage.cpp
    testPathChecker.cpp
    testReorder.cpp
    testSaveLayersDialog.cpp
    testSessionState.cpp
    testSharedStage.cpp
    testStageSelectorWidget.cpp
    testUsdSyntaxHighlighter.cpp
)

# layerEditorDCCFunctions is compiled directly (not via UsdLayerEditorLib link)
# to keep the registry self-contained inside this test DLL, avoiding ODR issues.
set(DCC_FN_SOURCES
    ${NEW_LE_LIB}/layerEditorDCCFunctions.cpp
)

# Legacy widget sources compiled directly into this test DLL. This gives the
# test a self-contained copy of the old editor, avoiding ODR conflicts with
# UsdLayerEditorLib, which exports the new editor's versions of the same classes.
set(LEGACY_SOURCES
    ../../mayaQtUtils.cpp
    ../../componentSaveWidget.cpp
    ../../dirtyLayersCountBadge.cpp
    ../../generatedIconButton.cpp
    ../../layerContentsWidget.cpp
    ../../layerEditorWidget.cpp
    ../../layerTreeItem.cpp
    ../../layerTreeItemDelegate.cpp
    ../../layerTreeModel.cpp
    ../../layerTreeView.cpp
    ../../loadLayersDialog.cpp
    ../../pathChecker.cpp
    ../../qtUtils.cpp
    ../../resources.qrc
    ../../saveLayersDialog.cpp
    ../../sessionState.cpp
    ../../stageSelectorWidget.cpp
    ../../stringResources.cpp
    ../../usdSyntaxHighlighter.cpp
    ../../warningDialogs.cpp
)

if(AdskUsdEditForward_FOUND)
    list(APPEND LEGACY_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../editForward/editForwardDialog.cpp)
endif()

# ── Target ─────────────────────────────────────────────────────────────────────
set(TARGET_NAME mayaUsdOldLayerEditorTests)

add_library(${TARGET_NAME} SHARED
    ${OLD_LE_OWN_SOURCES}
    ${DCC_FN_SOURCES}
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
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

# Include path order is critical:
#   1. OLD editor test dir FIRST — shim headers shadow new editor versions.
#   2. NEW editor test dir SECOND — *Logic.h headers and testUtils.h.
#   3. OLD editor lib THIRD — abstractCommandHook.h etc. must shadow the new lib versions.
#   4. NEW editor lib LAST — layerEditorDCCFunctions.h, only resolved if not found above.
target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NEW_LE_TEST_CPP}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
        ${NEW_LE_LIB}
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

install(FILES testOldLayerEditorParity.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/tests
)

# Register with CTest — run inside interactive Maya so QApplication and all
# Maya APIs are available for the old editor's widget tests.
if (BUILD_TESTS)
    mayaUsd_add_test(${TARGET_NAME}
        INTERACTIVE
        PYTHON_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/testOldLayerEditorParity.py
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
    )
    set_property(TEST ${TARGET_NAME} APPEND PROPERTY LABELS MayaUsdOldLEParity)
endif()
