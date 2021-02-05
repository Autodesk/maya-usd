#!/usr/bin/env mayapy
#
# Copyright 2016 Pixar
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

from pxr import Usd, Gf

from maya import cmds
from maya.api import OpenMaya as OM
from maya import standalone

import fixturesUtils

class testUsdImportXformAnim(unittest.TestCase):

    def _LoadUsd(self):
        # Import the USD file.
        usdFilePath = os.path.join(self.inputPath, "UsdImportXformAnimTest", "Mesh.usda")
        cmds.usdImport(file=usdFilePath, readAnimData=False)
        self.stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(self.stage)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.readOnlySetUpClass(__file__)

    def setUp(self):
        cmds.file(new=True, force=True)

    def assertEqualMatrix(self, m1, m2):
        self.assertEqual(len(m1), len(m2))
        for index, values in enumerate(zip(m1, m2)):
            self.assertAlmostEqual(values[0], values[1], 7, "Matrix differ at element %d: %f != %f" % (index, values[0], values[1]))

    def testUsdImport(self):
        """
        Tests a simple import of a file that contains an animated transform but loaded without anim.
        """
        self._LoadUsd()

        expectedWorldMatrix = [
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            5.0, 5.0, 5.0, 1.0
        ]
        actualWorldMatrix = cmds.getAttr("PolyMesh.worldMatrix")
        self.assertEqualMatrix(expectedWorldMatrix, actualWorldMatrix)


if __name__ == '__main__':
    unittest.main(verbosity=2)
