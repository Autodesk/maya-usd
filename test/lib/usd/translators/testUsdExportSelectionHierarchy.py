#!/usr/bin/env mayapy
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


import os
import unittest

from maya import cmds
from maya import standalone

from pxr import Usd

import fixturesUtils

class testUsdExportSelectionHierarchy(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportWithSelection(self):

        mayaFilePath = os.path.join(self.inputPath, "UsdExportSelectionHierarchy", "UsdExportSelectionHierarchy.ma")
        cmds.file(mayaFilePath, force=True, open=True)
  
        selectionInput = ['|pSphere1|pSphere3|pSphere9|pSphere19', '|pSphere1|pSphere3|pSphere7', '|pSphere1|pSphere3|pSphere9',
            '|pSphere1|pSphere2|pSphere4|pSphereShape4', '|pSphere1|pSphere2', '|pSphere1|pSphere3|pSphereShape3']

        cmds.select(selectionInput, af=True)

        selectionResult = cmds.ls(long=True, sl=True)

        # Want to make sure the order is maintatined
        self.assertTrue(selectionResult == selectionInput)

        usdFilePath = os.path.abspath('UsdExportSelectionHierarchy_EXPORTED.usda')
        cmds.mayaUSDExport(selection=True, file=usdFilePath, shadingMode='none')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        expectedExportedPrims = [
            '/pSphere1',
            '/pSphere1/pSphere2',
            '/pSphere1/pSphere2/pSphereShape2',
            '/pSphere1/pSphere2/pSphere4',
            '/pSphere1/pSphere2/pSphere5',
            '/pSphere1/pSphere2/pSphere6',
            '/pSphere1/pSphere2/pSphere4/pSphere10',
            '/pSphere1/pSphere2/pSphere4/pSphere11',
            '/pSphere1/pSphere2/pSphere4/pSphere12',
            '/pSphere1/pSphere2/pSphere4/pSphere13',
            '/pSphere1/pSphere2/pSphere4/pSphere14',
            '/pSphere1/pSphere2/pSphere4/pSphere15',
        ]

        expectedNonExportedPrims = [
            '/pSphere1/pSphereShape1',
            '/pSphere1/pSphere3/pSphere8',
        ]

        for primPath in expectedExportedPrims:
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim.IsValid())

        for primPath in expectedNonExportedPrims:
            prim = stage.GetPrimAtPath(primPath)
            self.assertFalse(prim.IsValid())

if __name__ == '__main__':
    unittest.main(verbosity=2)
