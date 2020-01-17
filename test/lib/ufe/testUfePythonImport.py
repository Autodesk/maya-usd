#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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

import unittest


class UfePythonImportTestCase(unittest.TestCase):
    """
    Verify that the ufe Python module imports correctly.
    """

    def testImportModule(self):
        from mayaUsd import ufe as mayaUsdUfe

        # Test calling a function that depends on USD. This exercises the
        # script module loader registry function and ensures that loading the
        # ufe library also loaded its dependencies (i.e. the core USD
        # libraries). We test the type name as a string to ensure that we're
        # not causing USD libraries to be loaded any other way.
        invalidPrim = mayaUsdUfe.getPrimFromRawItem(0)
        # XXX: Interestingly, it appears that a default constructed UsdPrim()
        # in C++ is currently returned to Python as a Usd.Object rather than a
        # Usd.Prim. Will follow up with core USD...
        self.assertEqual(type(invalidPrim).__name__, 'Object')
