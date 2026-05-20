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

# The same test sources compile into two executables.
# Only include paths and link targets differ — no #ifdef in test code.
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
)

# ------------------------------------------------------------------------------
# UsdLayerEditorOldTests
# Compiles the DCC-agnostic sources from lib/usd/ui/layerEditor/ directly.
# Does NOT link mayaUsdUI (which carries Maya SDK dependencies).
# ------------------------------------------------------------------------------
set(OLD_LE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../lib/usd/ui/layerEditor)

set(OLD_LE_SOURCES
    ${OLD_LE_DIR}/batchSaveLayersUIDelegate.cpp
    ${OLD_LE_DIR}/dirtyLayersCountBadge.cpp
    ${OLD_LE_DIR}/generatedIconButton.cpp
    ${OLD_LE_DIR}/layerContentsWidget.cpp
    ${OLD_LE_DIR}/layerEditorWidget.cpp
    ${OLD_LE_DIR}/layerTreeItem.cpp
    ${OLD_LE_DIR}/layerTreeItemDelegate.cpp
    ${OLD_LE_DIR}/layerTreeModel.cpp
    ${OLD_LE_DIR}/layerTreeView.cpp
    ${OLD_LE_DIR}/loadLayersDialog.cpp
    ${OLD_LE_DIR}/pathChecker.cpp
    ${OLD_LE_DIR}/qtUtils.cpp
    ${OLD_LE_DIR}/saveLayersDialog.cpp
    ${OLD_LE_DIR}/sessionState.cpp
    ${OLD_LE_DIR}/stageSelectorWidget.cpp
    ${OLD_LE_DIR}/stringResources.cpp
    ${OLD_LE_DIR}/usdSyntaxHighlighter.cpp
    ${OLD_LE_DIR}/warningDialogs.cpp
    ${OLD_LE_DIR}/resources.qrc
    # componentSaveWidget.cpp excluded: despite its generic name, it includes
    # maya/MGlobal.h and MayaUsd::utils — it is Maya-specific.
)

add_executable(UsdLayerEditorOldTests
    ${LAYER_EDITOR_TEST_SOURCES}
    ${OLD_LE_SOURCES}
)

mayaUsd_compile_config(UsdLayerEditorOldTests)

target_compile_definitions(UsdLayerEditorOldTests
    PRIVATE
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

target_include_directories(UsdLayerEditorOldTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${OLD_LE_DIR}
        ${UFE_INCLUDE_DIR}
        ${PXR_INCLUDE_DIRS}
)

target_link_libraries(UsdLayerEditorOldTests
    PRIVATE
        GTest::GTest
        Qt::Core Qt::Gui Qt::Widgets Qt::Test
        sdf tf usd usdUfe
        ${UFE_LIBRARY}
)

mayaUsd_add_test(UsdLayerEditorOldTests
    COMMAND $<TARGET_FILE:UsdLayerEditorOldTests>
    ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)

# ------------------------------------------------------------------------------
# UsdLayerEditorNewTests
# Links UsdLayerEditorLib. Only built when BUILD_NEW_LAYER_EDITOR=ON.
# ------------------------------------------------------------------------------
if(BUILD_NEW_LAYER_EDITOR)
    set(NEW_LE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)

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
            Qt::Core Qt::Gui Qt::Widgets Qt::Test
            sdf tf usd
            ${UFE_LIBRARY}
    )

    mayaUsd_add_test(UsdLayerEditorNewTests
        COMMAND $<TARGET_FILE:UsdLayerEditorNewTests>
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )
endif()
