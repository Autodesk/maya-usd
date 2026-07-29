#!/usr/bin/env mayapy
#
# Copyright 2026 Autodesk
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

from pxr import UsdGeom

import mayaUsd.lib as mayaUsdLib

from maya import cmds
from maya import standalone

import os
import unittest

import fixturesUtils

class testUsdImportAlembicReference(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.readOnlySetUpClass(__file__)

        usdFile = os.path.join(inputPath, "UsdImportAlembicReferenceTest", "referencing-cone.usda")
        cmds.usdImport(file=usdFile, shadingMode=[['none', 'default'], ])

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def verifyUVSet(self, mesh):
        self.assertTrue(cmds.objExists(mesh))
        self.assertEqual(cmds.getAttr(mesh + ".uvSet", size=True), 1)
        self.assertEqual(cmds.getAttr(mesh + ".uvSet[0].uvSetName"), "st")
        self.assertEqual(cmds.getAttr(mesh + ".uvSet[0].uvSetPoints", size=True), 42)
        self.assertAlmostEqual(cmds.getAttr(mesh + ".uvSet[0].uvSetPoints[0]")[0][0], 0.73776429, places=6)
        self.assertAlmostEqual(cmds.getAttr(mesh + ".uvSet[0].uvSetPoints[0]")[0][1], 0.1727457, places=6)
        self.assertAlmostEqual(cmds.getAttr(mesh + ".uvSet[0].uvSetPoints[1]")[0][0], 0.70225441, places=6)
        self.assertAlmostEqual(cmds.getAttr(mesh + ".uvSet[0].uvSetPoints[1]")[0][1], 0.103053599, places=6)
        self.assertAlmostEqual(cmds.getAttr(mesh + ".uvSet[0].uvSetPoints[41]")[0][0], 0.5, places=6)
        self.assertAlmostEqual(cmds.getAttr(mesh + ".uvSet[0].uvSetPoints[41]")[0][1], 1.0, places=6)

    def testImportPoly(self):
        self.verifyUVSet('refer')


if __name__ == '__main__':
    unittest.main(verbosity=2)
