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

# Old layer editor parity test suite. Temporary — deleted when the old editor
# is retired. See cpp/PARITY_STATUS.md for test coverage details.

add_subdirectory(cpp)

mayaUsd_get_unittest_target(target testOldLayerEditorParity.py)
mayaUsd_add_test(${target}
    INTERACTIVE
    PYTHON_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/testOldLayerEditorParity.py
    ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
)
set_property(TEST ${target} APPEND PROPERTY LABELS MayaUsdOldLEParity)

install(FILES testOldLayerEditorParity.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/tests
)
