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

# Top-level CMakeLists for the shared usdLayerEditor component (DCC-agnostic).

# Without MayaUsd (e.g. for 3dsMax) the editor installs into its own sub-folder
# so it can be packaged separately from the other libraries.
if(BUILD_MAYAUSD_LIBRARY)
    set(USDLAYEREDITOR_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
else()
    set(USDLAYEREDITOR_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/UsdLayerEditor)

    # Headers are installed flat so consumers use <UsdLayerEditor/xxx.h>.
    file(GLOB LAYER_EDITOR_PUBLIC_HEADERS lib/*.h ui/*.h)
    install(FILES ${LAYER_EDITOR_PUBLIC_HEADERS}
        DESTINATION ${USDLAYEREDITOR_INSTALL_PREFIX}/include/UsdLayerEditor
    )

    set(LAYER_EDITOR_TEST_DIR ${PROJECT_SOURCE_DIR}/test/lib/usdLayerEditor)
    install(FILES
        ${LAYER_EDITOR_TEST_DIR}/layer_editor_test.py
        ${LAYER_EDITOR_TEST_DIR}/test_utils.py
        DESTINATION ${USDLAYEREDITOR_INSTALL_PREFIX}/test
    )
    install(DIRECTORY ${LAYER_EDITOR_TEST_DIR}/data
        DESTINATION ${USDLAYEREDITOR_INSTALL_PREFIX}/test
    )
endif()

add_subdirectory(lib)
add_subdirectory(ui)
add_subdirectory(python)
