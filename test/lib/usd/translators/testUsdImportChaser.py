#!/usr/bin/env mayapy
#
# Copyright 2021 Apple
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
import maya.OpenMaya as om
import maya.cmds as cmds
import maya.standalone as mayastandalone
import os
import unittest
from pxr import Usd, Gf, UsdGeom


class TestUsdImportChaser(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.temp_dir = fixturesUtils.setUpClass(__file__)

    def setUp(self):
        self.stagePath = os.path.join(self.temp_dir, "UsdImportChaserTest", "importChaser.usda")
        stage = Usd.Stage.CreateNew(self.stagePath)
        UsdGeom.Xform.Define(stage, '/a')
        UsdGeom.Sphere.Define(stage, '/a/sphere')
        xformPrim = stage.GetPrimAtPath('/a')

        spherePrim = stage.GetPrimAtPath('/a/sphere')
        radiusAttr = spherePrim.GetAttribute('radius')
        radiusAttr.Set(2)
        extentAttr = spherePrim.GetAttribute('extent')
        extentAttr.Set(extentAttr.Get() * 2)
        stage.GetRootLayer().defaultPrim = 'a'
        stage.GetRootLayer().customLayerData = {"customKeyA" : "customValueA",
                                                 "customKeyB" : "customValueB"}
        stage.GetRootLayer().Save()

    @classmethod
    def tearDownClass(cls):
        mayastandalone.uninitialize()

    def testImportChaser(self):
        cmds.loadPlugin('usdTestMayaPlugin')  # NOTE: (yliangsiew) We load this plugin since the chaser is compiled as part of it.
        rootPaths = cmds.mayaUSDImport(v=True, f=self.stagePath, chaser=['info'])
        self.assertEqual(len(rootPaths), 1)
        sl = om.MSelectionList()
        sl.add(rootPaths[0])
        root = om.MObject()
        sl.getDependNode(0, root)
        fnNode = om.MFnDependencyNode(root)
        self.assertTrue(fnNode.hasAttribute("customData"))
        plgCustomData = fnNode.findPlug("customData")
        customDataStr = plgCustomData.asString()
        self.assertEqual(customDataStr, "Custom layer data: customKeyAcustomValueA\ncustomKeyBcustomValueB\n")


if __name__ == '__main__':
    unittest.main()
