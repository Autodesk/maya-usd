#!/usr/bin/env mayapy
#
# Copyright 2024 Autodesk
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

class testUsdExportMesh(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

        cls.testFile = os.path.join(cls.inputPath, "UsdExportTypeTest", "UsdExportTypeTest.ma")
        cmds.file(cls.testFile, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportDefaultPrim(self):
        cmds.file(self.testFile, force=True, open=True)
        usdFile = os.path.abspath('UsdExportDefaultPrim.usda')

        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', defaultPrim='pSphere1')
        
        stage = Usd.Stage.Open(usdFile)
        self.assertEqual(stage.GetDefaultPrim().GetName(), 'pSphere1')

        cmds.file(self.testFile, force=True, open=True)
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', defaultPrim='pointLight1')
        
        stage = Usd.Stage.Open(usdFile)
        self.assertEqual(stage.GetDefaultPrim().GetName(), 'pointLight1')

        # Test setting default prim with set parent scope on export
        cmds.file(self.testFile, force=True, open=True)
        usdFile = os.path.abspath('UsdExportDefaultPrim.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', rootPrim='testScope', defaultPrim = 'testScope')
        
        stage = Usd.Stage.Open(usdFile)
        self.assertEqual(stage.GetDefaultPrim().GetName(), 'testScope')


if __name__ == '__main__':
    unittest.main(verbosity=2)
