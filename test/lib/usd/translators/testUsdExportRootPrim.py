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

from pxr import Usd
from pxr import UsdGeom
from pxr import Vt
from pxr import Gf

import fixturesUtils

class testUsdExportRootPrim(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        filePath = os.path.join(inputPath, "UsdExportRootPrimTest", "UsdExportRootPrimTest.ma")
        cmds.file(filePath, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportRootPrim(self):
        usdFile = os.path.abspath('UsdExportRootPrim_testRootPrim.usda')
        cmds.usdExport(mergeTransformAndShape=False, exportInstances=False,
            shadingMode='none', rootPrim='testRoot', file=usdFile, frameRange=(1, 1))

        stage = Usd.Stage.Open(usdFile)

        p = UsdGeom.Mesh.Get(stage, '/testRoot/pSphere1/pSphereShape1')
        self.assertTrue(p.GetPrim().IsValid())

    def testExportRootPrimType(self):
        usdFile = os.path.abspath('UsdExportRootPrim_testRootPrim.usda')
        cmds.usdExport(mergeTransformAndShape=False, exportInstances=False,
            shadingMode='none', rootPrim='testRoot', rootPrimType='xform', file=usdFile, frameRange=(1, 1))

        stage = Usd.Stage.Open(usdFile)

        p = UsdGeom.Mesh.Get(stage, '/testRoot')
        self.assertEqual(p.GetPrim().GetPrimTypeInfo().GetTypeName(), "Xform")

    def testExportNoRootPrim(self):
        usdFile = os.path.abspath('UsdExportRootPrim_testNoRootPrim.usda')
        cmds.usdExport(mergeTransformAndShape=False, exportInstances=False,
            shadingMode='none', file=usdFile, frameRange=(1, 1))

        stage = Usd.Stage.Open(usdFile)

        p = UsdGeom.Mesh.Get(stage, '/pSphere1/pSphereShape1')
        self.assertTrue(p.GetPrim().IsValid())

if __name__ == '__main__':
    unittest.main(verbosity=2)
