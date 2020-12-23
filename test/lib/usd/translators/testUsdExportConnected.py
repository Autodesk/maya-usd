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

from maya import cmds
from maya import standalone

from pxr import Usd, UsdGeom

import fixturesUtils

class testUsdExportConnected(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportConnectedPlug(self):
        # tests issue #656

        # here, visibility was connected to another plug that was not animated.
        filePath = os.path.join(self.inputPath, "UsdExportConnectedTest", "visibility.ma")
        cmds.file(filePath, force=True, open=True)

        usdFile = os.path.abspath("visibility.usda")
        cmds.usdExport(file=usdFile, exportVisibility=True, shadingMode='none')

        stage = Usd.Stage.Open(usdFile)

        p = stage.GetPrimAtPath('/driven')
        self.assertEqual(UsdGeom.Imageable(p).GetVisibilityAttr().Get(), 'invisible')

        p = stage.GetPrimAtPath('/invised')
        self.assertEqual(UsdGeom.Imageable(p).GetVisibilityAttr().Get(), 'invisible')

if __name__ == '__main__':
    unittest.main(verbosity=2)
