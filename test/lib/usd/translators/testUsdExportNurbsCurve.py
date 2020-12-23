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

from maya import cmds
from maya import standalone

from pxr import Gf
from pxr import Usd
from pxr import UsdGeom
from pxr import Vt

import fixturesUtils

class testUsdExportNurbsCurve(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        filePath = os.path.join(inputPath, "UsdExportNurbsCurveTest", "UsdExportNurbsCurveTest.ma")
        cmds.file(filePath, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExport(self):
        '''
        Export the Maya file and validate a handful of properties.
        '''
        usdFile = os.path.abspath('UsdExportNurbsCurveTest.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none')

        stage = Usd.Stage.Open(usdFile)
        
        nc = UsdGeom.NurbsCurves.Get(stage, '/curve1')
        self.assertEqual(nc.GetWidthsAttr().Get(), Vt.FloatArray([1.0]))
        self.assertEqual(nc.GetWidthsInterpolation(), UsdGeom.Tokens.constant)
        self.assertEqual(nc.GetRangesAttr().Get(), Vt.Vec2dArray([Gf.Vec2d(0, 5)]))
        self.assertEqual(nc.GetOrderAttr().Get(), Vt.IntArray([4]))
        self.assertEqual(nc.GetCurveVertexCountsAttr().Get(), Vt.IntArray([8]))


if __name__ == '__main__':
    unittest.main(verbosity=2)
