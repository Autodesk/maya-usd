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

# Path to the new editor's test/cpp — contains *Logic.h headers.
set(NEW_LE_TEST_CPP
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../usdUfe/usd-layer-editor/test/cpp)

# ── Sources ────────────────────────────────────────────────────────────────────
set(OLD_LE_OWN_SOURCES
    # Headers-only so AUTOMOC generates staticMetaObject for Q_OBJECT classes
    # not exported as data members from mayaUsdUI.dll.
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sessionState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../layerContentsWidget.h
    stubCommandHook.cpp
    stubSessionState.cpp
    testFixture.cpp
    testMain.cpp
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

# ── Target ─────────────────────────────────────────────────────────────────────
set(TARGET_NAME mayaUsdOldLayerEditorTests)

add_executable(${TARGET_NAME}
    ${OLD_LE_OWN_SOURCES}
)

mayaUsd_compile_config(${TARGET_NAME})

target_compile_definitions(${TARGET_NAME}
    PRIVATE
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

# Include path order is critical:
#   1. OLD editor test dir FIRST — shim headers shadow new editor versions.
#   2. NEW editor test dir SECOND — *Logic.h headers and testUtils.h.
#   3. OLD editor lib sources — layerTreeItem.h, layerTreeModel.h, etc.
target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NEW_LE_TEST_CPP}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        mayaUsdUI
        GTest::GTest
        Qt::Core Qt::Gui Qt::Widgets
        sdf tf usd
        ${MAYA_LIBRARIES}
)

install(TARGETS ${TARGET_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/tests
)

# Register with CTest — run as a standalone executable (no Maya needed).
mayaUsd_add_test(${TARGET_NAME}
    COMMAND $<TARGET_FILE:${TARGET_NAME}>
    ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        "QT_QPA_PLATFORM=offscreen"
)
set_property(TEST ${TARGET_NAME} APPEND PROPERTY LABELS MayaUsdOldLEParity)
