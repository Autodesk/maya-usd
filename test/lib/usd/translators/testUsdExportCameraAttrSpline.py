#!/usr/bin/env mayapy
#
# Copyright 2025 Autodesk
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
from pxr import UsdGeom
from pxr import Sdf
from pxr import Gf

from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdExportCameraAttrSpline(unittest.TestCase):

    EPSILON = 1e-3

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        filePath = os.path.join(inputPath, "UsdExportCameraTest", "UsdExportCameraAttrSplineTest.ma")
        cmds.file(filePath, force=True, open=True)

        # Export to USD.
        usdFilePath = os.path.abspath('UsdExportCameraAttrSplineTest.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none',
            animationType='curves')

        cls.stage = Usd.Stage.Open(usdFilePath)

    @unittest.skipUnless(Usd.GetVersion() >= (0, 24, 11), 'Splines are only supported in USD 0.24.11 and later')
    def testStagePrerequisites(self):
        self.assertTrue(self.stage)

    def _GetUsdCamera(self, cameraName):
        cameraPrimPath = '/cameras/%s' % cameraName
        cameraPrim = self.stage.GetPrimAtPath(cameraPrimPath)
        self.assertTrue(cameraPrim)

        usdCamera = UsdGeom.Camera(cameraPrim)
        self.assertTrue(usdCamera)

        return usdCamera

    @unittest.skipUnless(Usd.GetVersion() >= (0, 24, 11), 'Splines are only supported in USD 0.24.11 and later')
    def testExportPerspectiveAttributes(self):
        """
        Tests exporting perspective camera's attributes with splines.
        """
        from pxr import Ts
        usdCamera = self._GetUsdCamera('perspCamera')

        horizontalOffsetAttr = usdCamera.GetPrim().GetAttribute('horizontalApertureOffset')
        spline = horizontalOffsetAttr.GetSpline()
        self.assertTrue(spline)

        knots = spline.GetKnots()

        self.assertEqual(spline.GetPreExtrapolation().mode, Ts.ExtrapLoopRepeat)
        self.assertEqual(spline.GetPostExtrapolation().mode, Ts.ExtrapLoopOscillate)

        self.assertEqual(knots.GetValueType(), str(Sdf.ValueTypeNames.Float))
        self.assertEqual(len(knots), 2)
        self.assertEqual(knots[0].GetTime(), 0)
        self.assertTrue(Gf.IsClose(knots[0].GetValue(), 50.8, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[0].GetPreTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[0].GetPostTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertEqual(knots[0].GetPostTanSlope(), 0)
        self.assertEqual(knots[0].GetPreTanSlope(), 0)

        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 203.2, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[5].GetPostTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[5].GetPreTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertEqual(knots[5].GetPostTanSlope(), 0)
        self.assertEqual(knots[5].GetPreTanSlope(), 0)

        verticalOffsetAttr = usdCamera.GetPrim().GetAttribute('verticalApertureOffset')
        spline = verticalOffsetAttr.GetSpline()
        self.assertTrue(spline)

        knots = spline.GetKnots()

        self.assertEqual(knots.GetValueType(), str(Sdf.ValueTypeNames.Float))
        self.assertEqual(len(knots), 2)
        self.assertEqual(knots[0].GetTime(), 0)
        self.assertTrue(Gf.IsClose(knots[0].GetValue(), 50.8, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[0].GetPreTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[0].GetPostTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertEqual(knots[0].GetPostTanSlope(), 0)
        self.assertEqual(knots[0].GetPreTanSlope(), 0)

        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 203.2, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[5].GetPostTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[5].GetPreTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertEqual(knots[5].GetPostTanSlope(), 0)
        self.assertEqual(knots[5].GetPreTanSlope(), 0)

        # Validate non-spline attributes
        clippingRangeAttr = usdCamera.GetPrim().GetAttribute('clippingRange')
        clippingRange = clippingRangeAttr.Get()
        self.assertTrue(Gf.IsClose(clippingRange[0], 0.1, self.EPSILON))
        self.assertTrue(Gf.IsClose(clippingRange[1], 10000, self.EPSILON))

        focalLengthAttr = usdCamera.GetPrim().GetAttribute('focalLength')
        focalLength = focalLengthAttr.Get()
        self.assertTrue(Gf.IsClose(focalLength, 35, self.EPSILON))

        focusDistanceAttr = usdCamera.GetPrim().GetAttribute('focusDistance')
        focusDistance = focusDistanceAttr.Get()
        self.assertTrue(Gf.IsClose(focusDistance, 5.0465837, self.EPSILON))

        fStopAttr = usdCamera.GetPrim().GetAttribute('fStop')
        fStop = fStopAttr.Get()
        self.assertTrue(Gf.IsClose(fStop, 1.7974684, self.EPSILON))

        horizontalApertureAttr = usdCamera.GetPrim().GetAttribute('horizontalAperture')
        horizontalAperture = horizontalApertureAttr.Get()
        self.assertTrue(Gf.IsClose(horizontalAperture, 35.999928, self.EPSILON))

        verticalApertureAttr = usdCamera.GetPrim().GetAttribute('verticalAperture')
        verticalAperture = verticalApertureAttr.Get()
        self.assertTrue(Gf.IsClose(verticalAperture, 23.999952, self.EPSILON))

    @unittest.skipUnless(Usd.GetVersion() >= (0, 24, 11), 'Splines are only supported in USD 0.24.11 and later')
    def testExportOrthographicAttributes(self):
        """
        Tests exporting ortographic camera's attributes with splines.
        """
        from pxr import Ts
        usdCamera = self._GetUsdCamera('orthoCamera')

        focalLengthAttr = usdCamera.GetPrim().GetAttribute('focalLength')
        spline = focalLengthAttr.GetSpline()
        self.assertTrue(spline)

        knots = spline.GetKnots()

        self.assertEqual(knots.GetValueType(), str(Sdf.ValueTypeNames.Float))
        self.assertEqual(len(knots), 3)
        self.assertEqual(knots[0].GetTime(), 0)
        self.assertEqual(knots[0].GetValue(), 35)
        self.assertEqual(knots[0].GetPreTanWidth(), 1)
        self.assertTrue(Gf.IsClose(knots[0].GetPostTanWidth(), 1, self.EPSILON))
        self.assertEqual(knots[0].GetPostTanSlope(), 0)
        self.assertEqual(knots[0].GetPreTanSlope(), 0)

        self.assertEqual(knots[3].GetTime(), 3)
        self.assertTrue(Gf.IsClose(knots[3].GetValue(), 89.22748, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[3].GetPreTanWidth(), 1, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[3].GetPostTanWidth(), 0.6666666666666666, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[3].GetPostTanSlope(), -18.585257, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[3].GetPreTanSlope(), -18.585245, self.EPSILON))

        self.assertEqual(knots[5].GetTime(), 5)
        self.assertEqual(knots[5].GetValue(), 2.5)
        self.assertTrue(Gf.IsClose(knots[5].GetPreTanWidth(), 0.6666666666666666, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[5].GetPostTanWidth(), 0.6666666666666666, self.EPSILON))

        focusDistanceAttr = usdCamera.GetPrim().GetAttribute('focusDistance')
        spline = focusDistanceAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()

        self.assertEqual(knots.GetValueType(), str(Sdf.ValueTypeNames.Float))
        self.assertEqual(len(knots), 3)
        self.assertEqual(knots[0].GetTime(), 0)
        self.assertTrue(Gf.IsClose(knots[0].GetValue(), 5.0465837, self.EPSILON))
        self.assertEqual(knots[0].GetPreTanWidth(), 8)
        self.assertTrue(Gf.IsClose(knots[0].GetPostTanWidth(), 0.6666666666666666, self.EPSILON))
        self.assertEqual(knots[0].GetPostTanSlope(), 0)
        self.assertEqual(knots[0].GetPreTanSlope(), 0)

        self.assertEqual(knots[2].GetTime(), 2)
        self.assertTrue(Gf.IsClose(knots[2].GetValue(), 7.160621, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[2].GetPreTanWidth(), 0.6666666666666666, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[2].GetPostTanWidth(), 1, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[2].GetPostTanSlope(), 0.36788943, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[2].GetPreTanSlope(), 0.36788934, self.EPSILON))
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 10.046584, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[5].GetPreTanWidth(), 1, self.EPSILON))
        self.assertEqual(knots[5].GetPostTanWidth(), 8)

        fStopAttr = usdCamera.GetPrim().GetAttribute('fStop')
        spline = fStopAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()

        self.assertEqual(knots.GetValueType(), str(Sdf.ValueTypeNames.Float))
        self.assertEqual(len(knots), 2)
        self.assertEqual(knots[0].GetTime(), 0)
        self.assertTrue(Gf.IsClose(knots[0].GetValue(), 1.7974684, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[0].GetPreTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[0].GetPostTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertEqual(knots[0].GetPostTanSlope(), 0)
        self.assertEqual(knots[0].GetPreTanSlope(), 0)
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 7.778481, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[5].GetPreTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[5].GetPostTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertEqual(knots[5].GetPostTanSlope(), 0)
        self.assertEqual(knots[5].GetPreTanSlope(), 0)

        horizontalApertureAttr = usdCamera.GetPrim().GetAttribute('horizontalAperture')
        spline = horizontalApertureAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()

        self.assertEqual(knots.GetValueType(), str(Sdf.ValueTypeNames.Float))
        self.assertEqual(len(knots), 2)
        self.assertEqual(knots[0].GetTime(), 0)
        self.assertTrue(Gf.IsClose(knots[0].GetValue(), 100, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[0].GetPreTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[0].GetPostTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertEqual(knots[0].GetPostTanSlope(), 0)
        self.assertEqual(knots[0].GetPreTanSlope(), 0)
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 300, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[5].GetPreTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[5].GetPostTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertEqual(knots[5].GetPostTanSlope(), 0)
        self.assertEqual(knots[5].GetPreTanSlope(), 0)

        verticalApertureAttr = usdCamera.GetPrim().GetAttribute('verticalAperture')
        spline = verticalApertureAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()

        self.assertEqual(knots.GetValueType(), str(Sdf.ValueTypeNames.Float))
        self.assertEqual(len(knots), 2)
        self.assertEqual(knots[0].GetTime(), 0)
        self.assertTrue(Gf.IsClose(knots[0].GetValue(), 100, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[0].GetPreTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[0].GetPostTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertEqual(knots[0].GetPostTanSlope(), 0)
        self.assertEqual(knots[0].GetPreTanSlope(), 0)
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 300, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[5].GetPreTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertTrue(Gf.IsClose(knots[5].GetPostTanWidth(), 1.6666666666666667, self.EPSILON))
        self.assertEqual(knots[5].GetPostTanSlope(), 0)
        self.assertEqual(knots[5].GetPreTanSlope(), 0)

        projectionAttr = usdCamera.GetPrim().GetAttribute('projection')
        self.assertEqual(projectionAttr.Get(), UsdGeom.Tokens.orthographic)

if __name__ == '__main__':
    unittest.main(verbosity=2)