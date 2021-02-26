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

import fixturesUtils

from maya import standalone

import unittest


class UfePythonImportTestCase(unittest.TestCase):
    """
    Verify that the ufe Python module imports correctly.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testImportModule(self):
        from mayaUsd import ufe as mayaUsdUfe
        import ufe

        # Test calling a function that depends on USD. This exercises the
        # script module loader registry function and ensures that loading the
        # ufe library also loaded its dependencies (i.e. the core USD
        # libraries). We test the type name as a string to ensure that we're
        # not causing USD libraries to be loaded any other way.
        if ufe.VersionInfo.getMajorVersion() == 1:
            # In UFE v1 if the Nb segments == 1, returns invalid prim
            invalidPrim = mayaUsdUfe.ufePathToPrim('a')
        else:
            # raw item only introduced in UFE v2
            invalidPrim = mayaUsdUfe.getPrimFromRawItem(0)

        # Prior to USD version 20.05, a default constructed UsdPrim() in C++
        # would be returned to Python as a Usd.Object rather than a Usd.Prim.
        # Since we still want to support earlier versions, make sure it's one
        # of the two.
        typeName = type(invalidPrim).__name__
        self.assertIn(typeName, ['Prim', 'Object'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
