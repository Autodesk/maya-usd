#!/usr/bin/env mayapy
#
# Copyright 2025 Autodesk
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


import os
import unittest

from maya import cmds
from maya import standalone

from pxr import Usd

import fixturesUtils

class testUsdExportAndHide(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportAndHide(self):

        mayaFilePath = os.path.join(self.inputPath, "UsdExportSelectionTest", "UsdExportSelectionTest.ma")
        cmds.file(mayaFilePath, force=True, open=True)
  
        selection = ['GroupA', 'Cube3', 'Cube6']
        cmds.select(selection)

        usdFilePath = os.path.abspath('UsdExportSelectionTest_EXPORTED.usda')
        cmds.usdExport(mergeTransformAndShape=True, selection=True,
            file=usdFilePath, shadingMode='none', hideSourceData=True)

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        expectedExportedPrims = [
            '/UsdExportSelectionTest',
            '/UsdExportSelectionTest/Geom',
            '/UsdExportSelectionTest/Geom/GroupA',
            '/UsdExportSelectionTest/Geom/GroupA/Cube1',
            '/UsdExportSelectionTest/Geom/GroupA/Cube2',
            '/UsdExportSelectionTest/Geom/GroupB',
            '/UsdExportSelectionTest/Geom/GroupB/Cube3',
            '/UsdExportSelectionTest/Geom/GroupC',
            '/UsdExportSelectionTest/Geom/GroupC/GroupD',
            '/UsdExportSelectionTest/Geom/GroupC/GroupD/Cube6',
        ]

        expectedNonExportedPrims = [
            '/UsdExportSelectionTest/Geom/GroupB/Cube4',
            '/UsdExportSelectionTest/Geom/GroupC/Cube5',
            '/UsdExportSelectionTest/Geom/GroupC/GroupD/Cube7',
        ]

        for primPath in expectedExportedPrims:
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim.IsValid())

        for primPath in expectedNonExportedPrims:
            prim = stage.GetPrimAtPath(primPath)
            self.assertFalse(prim.IsValid())

        visible_nodes = cmds.ls(visible=True)

        self.assertNotIn('Cube1', visible_nodes)
        self.assertNotIn('Cube2', visible_nodes)
        self.assertNotIn('Cube3', visible_nodes)
        self.assertNotIn('Cube6', visible_nodes)
        self.assertNotIn('GroupA', visible_nodes)

        self.assertIn('Cube4', visible_nodes)
        self.assertIn('Cube5', visible_nodes)


if __name__ == '__main__':
    unittest.main(verbosity=2)
