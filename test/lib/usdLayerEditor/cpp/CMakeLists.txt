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

set(LAYER_EDITOR_TEST_SOURCES
    testMain.cpp
    stubCommandHook.cpp
    stubSessionState.cpp
    testFixture.cpp
    testButtons.cpp
    testContextMenu.cpp
    testReorder.cpp
    testMenusAndStage.cpp
    testLayerTreeItem.cpp
    testLayerTreeModel.cpp
    testLayerTreeView.cpp
    testLayerContentsWidget.cpp
    testSaveLayersDialog.cpp
    testLoadLayersDialog.cpp
    testLayerLocking.cpp
    testLayerMuting.cpp
    testSharedStage.cpp
    testLayerEditorCommands.cpp
    testEFMode.cpp
    testDCCFunctions.cpp
    testFileSystemUtils.cpp
    testPathChecker.cpp
    testSerializationUtils.cpp
    testLayerTreeItemDelegate.cpp
    testComponentSaveWidget.cpp
)

# ------------------------------------------------------------------------------
# UsdLayerEditorNewTests
# Links UsdLayerEditorLib (the DCC-agnostic shared component).
# ------------------------------------------------------------------------------
set(NEW_LE_DIR ${CMAKE_SOURCE_DIR}/lib/usdLayerEditor/lib)

add_executable(UsdLayerEditorNewTests
    ${LAYER_EDITOR_TEST_SOURCES}
)

mayaUsd_compile_config(UsdLayerEditorNewTests)

target_compile_definitions(UsdLayerEditorNewTests
    PRIVATE
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

target_include_directories(UsdLayerEditorNewTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NEW_LE_DIR}
        ${UFE_INCLUDE_DIR}
)

target_link_libraries(UsdLayerEditorNewTests
    PRIVATE
        UsdLayerEditorLib
        GTest::GTest
        Qt::Core Qt::Gui Qt::Widgets
        sdf tf usd
        ${UFE_LIBRARY}
)

mayaUsd_add_test(UsdLayerEditorNewTests
    COMMAND $<TARGET_FILE:UsdLayerEditorNewTests>
    ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
