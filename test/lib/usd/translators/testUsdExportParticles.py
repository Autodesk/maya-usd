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

class testUsdExportParticles(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        filePath = os.path.join(inputPath, "UsdExportParticlesTest", "UsdExportParticlesTest.ma")
        cmds.file(filePath, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportInstances(self):
        usdFile = os.path.abspath('UsdExportParticles_particles.usda')
        cmds.usdExport(mergeTransformAndShape=False, exportInstances=False,
            shadingMode='none', file=usdFile, frameRange=(1, 1))

        stage = Usd.Stage.Open(usdFile)

        p = UsdGeom.Points.Get(stage, '/particle1/particleShape1')
        self.assertTrue(p.GetPrim().IsValid())
        self.assertEqual(p.GetPointsAttr().Get(1), Vt.Vec3fArray(5, (Gf.Vec3f(5.0, 0.0, -6.0), Gf.Vec3f(-4.0, 0.0, -4.5), Gf.Vec3f(-4.5, 0.0, 3.0), Gf.Vec3f(-2.0, 0.0, 8.5), Gf.Vec3f(3.0, 0.0, 3.5))))
        self.assertEqual(p.GetVelocitiesAttr().Get(1), Vt.Vec3fArray(5, (Gf.Vec3f(0.0, 0.0, 0.0), Gf.Vec3f(1.0, 1.0, 1.0), Gf.Vec3f(2.0, 3.0, 4.0), Gf.Vec3f(5.0, 6.0, 7.0), Gf.Vec3f(8.0, 9.0, 10.0))))
        self.assertEqual(p.GetWidthsAttr().Get(1), Vt.FloatArray(5, (2.0, 2.0, 2.0, 2.0, 2.0)))
        self.assertEqual(p.GetIdsAttr().Get(1), Vt.Int64Array(5, (0, 1, 2, 3, 4)))

if __name__ == '__main__':
    unittest.main(verbosity=2)
