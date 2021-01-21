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

from pxr import Kind
from pxr import Usd
from pxr import UsdGeom
from pxr import UsdUtils

import fixturesUtils

class testUsdExportPref(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        filePath = os.path.join(inputPath, "UsdExportPrefTest", "UsdExportPrefTest.ma")
        cmds.file(filePath, open=True, force=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportInstances(self):
        usdFile = os.path.abspath('UsdExportPref_nopref.usda')
        cmds.usdExport(mergeTransformAndShape=True, exportReferenceObjects=False,
            shadingMode='none', file=usdFile)

        stage = Usd.Stage.Open(usdFile)

        plane1Path = '/pPlane1'
        plane2Path = '/pPlane2'

        plane1 = UsdGeom.Mesh.Get(stage, plane1Path)
        self.assertTrue(plane1.GetPrim().IsValid())
        plane2 = UsdGeom.Mesh.Get(stage, plane2Path)
        self.assertTrue(plane2.GetPrim().IsValid())

        self.assertFalse(plane1.GetPrimvar(UsdUtils.GetPrefName()).IsDefined())

        usdFile = os.path.abspath('UsdExportPref_pref.usda')
        cmds.usdExport(mergeTransformAndShape=True, exportReferenceObjects=True,
            shadingMode='none', file=usdFile)

        stage = Usd.Stage.Open(usdFile)

        plane1 = UsdGeom.Mesh.Get(stage, plane1Path)
        self.assertTrue(plane1.GetPrim().IsValid())
        plane2 = UsdGeom.Mesh.Get(stage, plane2Path)
        self.assertTrue(plane2.GetPrim().IsValid())

        self.assertTrue(plane1.GetPrimvar(UsdUtils.GetPrefName()).IsDefined())
        self.assertEqual(plane1.GetPrimvar(UsdUtils.GetPrefName()).Get(), plane2.GetPointsAttr().Get())

if __name__ == '__main__':
    unittest.main(verbosity=2)
