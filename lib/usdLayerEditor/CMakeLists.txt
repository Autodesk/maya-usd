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

# lib/ is DCC- and Qt-agnostic and is linked by mayaUsd unconditionally, so it
# must be available even in a build without Qt. Only the widgets and their
# Python bindings need Qt.
add_subdirectory(lib)
if (Qt_FOUND)
    add_subdirectory(ui)
    add_subdirectory(python)
endif()
