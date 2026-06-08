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

# C++ GTest suite for the shared usdLayerEditor component.
add_subdirectory(cpp)

# Maya-driven interactive Python test that runs the DCC-agnostic
# layer_editor_test.py suite. Gated on BUILD_NEW_LAYER_EDITOR because it
# requires the UsdLayerEditorLib Python bindings to be installed.
if(BUILD_NEW_LAYER_EDITOR)
    mayaUsd_get_unittest_target(target testMayaUsdSharedLayerEditor.py)
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PYTHON_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/testMayaUsdSharedLayerEditor.py
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}"
    )
    set_property(TEST ${target} APPEND PROPERTY LABELS MayaUsd)
endif()
