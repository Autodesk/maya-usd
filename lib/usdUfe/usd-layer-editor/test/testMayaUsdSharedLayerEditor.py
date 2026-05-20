#!/usr/bin/env python
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
"""ctest entry point: runs the shared layer_editor_test.py suite inside Maya.

Registered in test/lib/CMakeLists.txt and discovered by ctest. This file is a
thin wrapper: it binds the Maya implementations of the DCC hooks (via
mayaLayerEditorTestSetup.setup()) and re-exports UsdLayerEditorTest so
unittest discovery picks up every test_* method from the shared component's
layer_editor_test.py.
"""

import os
import sys

# Make sure the shared component's test directory (this file's own dir) is
# importable so layer_editor_test and mayaLayerEditorTestSetup resolve.
_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)

import fixturesUtils  # noqa: E402
import mayaLayerEditorTestSetup  # noqa: E402
mayaLayerEditorTestSetup.setup()

# Re-export the shared test class so fixturesUtils.runTests(globals()) loads
# every test_* method from the shared component's layer_editor_test.py.
from layer_editor_test import UsdLayerEditorTest  # noqa: E402,F401


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
