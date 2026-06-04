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

# Path to the new editor's test/cpp — contains *Logic.h headers and the 7 as-is sources.
set(NEW_LE_TEST_CPP
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../usdUfe/usd-layer-editor/test/cpp)

# ── Sources ────────────────────────────────────────────────────────────────────
# Old-editor-specific files (stubs, fixture, thin wrappers, plugin host):
set(OLD_LE_OWN_SOURCES
    # Headers-only sources so AUTOMOC generates staticMetaObject for
    # classes whose Q_OBJECT data is not exported from mayaUsdUI.dll.
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sessionState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../layerContentsWidget.h
    stubCommandHook.cpp
    stubSessionState.cpp
    testFixture.cpp
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
    pluginMain.cpp
)

# ── Target ─────────────────────────────────────────────────────────────────────
set(TARGET_NAME mayaUsdOldLayerEditorTests)

add_library(${TARGET_NAME} MODULE
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
#   1. OLD editor test dir FIRST — shim headers (layerLocking.h, layerMuting.h,
#      customLayerData.h, testFixture.h) shadow the new editor's versions.
#   2. NEW editor test dir SECOND — *Logic.h headers and testUtils.h found here.
#   3. OLD editor lib sources — layerTreeItem.h, layerTreeModel.h, layerTreeView.h, etc.
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

# Maya plugin output conventions
set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "")
if(IS_WINDOWS)
    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".mll")
else()
    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".so")
endif()

install(TARGETS ${TARGET_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/maya
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/maya
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/testOldLayerEditorParity.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python
)

# Register with CTest via the Maya Python runner.
mayaUsd_get_unittest_target(parity_target testOldLayerEditorParity.py)
mayaUsd_add_test(${parity_target}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    PYTHON_MODULE ${parity_target}
    ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
        "QT_QPA_PLATFORM=offscreen"
)
set_property(TEST ${parity_target} APPEND PROPERTY LABELS MayaUsdOldLEParity)
