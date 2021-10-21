#!/usr/bin/env mayapy
#
# Copyright 2018 Pixar
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

from pxr import Usd

from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdExportOpenLayer(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportToDiskLayer(self):
        """
        Tests that exporting to an on-disk layer that is open elsewhere in the
        process still works.
        """
        cmds.file(new=True, force=True)
        cmds.polyCube(name='TestCube')

        filePath = os.path.join(self.temp_dir, "UsdExportNurbsCurveTest", "testStage.usda")

        stage = Usd.Stage.CreateNew(filePath)
        stage.Save()
        self.assertFalse(stage.GetPrimAtPath('/TestCube').IsValid())

        cmds.usdExport(
                file=filePath,
                mergeTransformAndShape=True,
                shadingMode='none')
        self.assertTrue(stage.GetPrimAtPath('/TestCube').IsValid())

        cmds.rename('TestCube', 'TestThing')
        cmds.usdExport(
                file=filePath,
                mergeTransformAndShape=True,
                shadingMode='none')
        self.assertFalse(stage.GetPrimAtPath('/TestCube').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/TestThing').IsValid())

    def testExportToAnonymousLayer(self):
        """
        Tests exporting to an existing anonymous layer. In normal (non-append)
        mode, this should completely overwrite the contents of the anonymous
        layer.
        """
        cmds.file(new=True, force=True)
        cmds.polyCube(name='TestCube')

        stage = Usd.Stage.CreateInMemory()
        self.assertFalse(stage.GetPrimAtPath('/TestCube').IsValid())

        cmds.usdExport(
                file=stage.GetRootLayer().identifier,
                mergeTransformAndShape=True,
                shadingMode='none')
        self.assertTrue(stage.GetPrimAtPath('/TestCube').IsValid())

        cmds.rename('TestCube', 'TestThing')
        cmds.usdExport(
                file=stage.GetRootLayer().identifier,
                mergeTransformAndShape=True,
                shadingMode='none')
        self.assertFalse(stage.GetPrimAtPath('/TestCube').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/TestThing').IsValid())


if __name__ == '__main__':
    unittest.main(verbosity=2)
