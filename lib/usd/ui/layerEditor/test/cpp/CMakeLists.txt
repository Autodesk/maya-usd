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

# ── Sources ────────────────────────────────────────────────────────────────────
set(OLD_LE_OWN_SOURCES
    stubCommandHook.cpp
    stubSessionState.cpp
    testFixture.cpp
    pluginMain.cpp
    testButtons.cpp
    testContextMenu.cpp
    testLayerContentsWidget.cpp
    testLayerLocking.cpp
    testLayerMuting.cpp
    testLayerTreeItem.cpp
    testLayerTreeModel.cpp
    testLayerTreeView.cpp
    testLoadLayersDialog.cpp
    testMenusAndStage.cpp
    testReorder.cpp
    testSaveLayersDialog.cpp
    testSharedStage.cpp
)

# Legacy widget sources compiled directly into this test DLL. This gives the
# test a self-contained copy of the old editor, avoiding ODR conflicts with
# UsdLayerEditorLib, which exports the new editor's versions of the same classes.
set(LEGACY_SOURCES
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

# ── Target ─────────────────────────────────────────────────────────────────────
set(TARGET_NAME mayaUsdOldLayerEditorTests)

add_library(${TARGET_NAME} SHARED
    ${OLD_LE_OWN_SOURCES}
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

target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        mayaUsd
        GTest::GTest
        Qt::Core Qt::Gui Qt::Widgets
        sdf tf usd
        ${MAYA_LIBRARIES}
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
