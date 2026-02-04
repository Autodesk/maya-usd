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
import math

from pxr import Gf
from pxr import Usd
from pxr import UsdGeom

import fixturesUtils
from maya import cmds
from maya import standalone


class testUsdExportSplineXforms(unittest.TestCase):

    START_TIMECODE = 1.0
    END_TIMECODE = 10.0
    EPSILON = 1e-3

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)
        # Create a new Maya scene
        cmds.file(new=True, force=True)
        
        # Create sphere and set initial position
        sphereResult = cmds.polySphere(name='animatedSphere', radius=1)
        sphere = sphereResult[0]  # Get the transform node
        
        # Frame 1:

        cmds.setKeyframe('%s.translateX' % sphere, time=cls.START_TIMECODE, value=0)
        cmds.setKeyframe('%s.translateY' % sphere, time=cls.START_TIMECODE, value=0)
        cmds.setKeyframe('%s.translateZ' % sphere, time=cls.START_TIMECODE, value=0)
        cmds.setKeyframe('%s.rotateX' % sphere, time=cls.START_TIMECODE, value=0)
        cmds.setKeyframe('%s.rotateY' % sphere, time=cls.START_TIMECODE, value=0)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=cls.START_TIMECODE, value=0)
        cmds.setKeyframe('%s.scaleX' % sphere, time=cls.START_TIMECODE, value=1)
        cmds.setKeyframe('%s.scaleY' % sphere, time=cls.START_TIMECODE, value=1)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=cls.START_TIMECODE, value=1)
        
        # Frame 2: 

        cmds.setKeyframe('%s.translateX' % sphere, time=2, value=5)
        cmds.setKeyframe('%s.translateY' % sphere, time=2, value=0)
        cmds.setKeyframe('%s.translateZ' % sphere, time=2, value=0)
        cmds.setKeyframe('%s.rotateX' % sphere, time=2, value=0)
        cmds.setKeyframe('%s.rotateY' % sphere, time=2, value=45)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=2, value=0)
        cmds.setKeyframe('%s.scaleX' % sphere, time=2, value=1.5)
        cmds.setKeyframe('%s.scaleY' % sphere, time=2, value=1.5)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=2, value=1.5)
        
        # Frame 3: 

        cmds.setKeyframe('%s.translateX' % sphere, time=3, value=5)
        cmds.setKeyframe('%s.translateY' % sphere, time=3, value=5)
        cmds.setKeyframe('%s.translateZ' % sphere, time=3, value=0)
        cmds.setKeyframe('%s.rotateX' % sphere, time=3, value=30)
        cmds.setKeyframe('%s.rotateY' % sphere, time=3, value=90)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=3, value=0)
        cmds.setKeyframe('%s.scaleX' % sphere, time=3, value=2)
        cmds.setKeyframe('%s.scaleY' % sphere, time=3, value=1)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=3, value=1)
        
        # Frame 4: 

        cmds.setKeyframe('%s.translateX' % sphere, time=4, value=0)
        cmds.setKeyframe('%s.translateY' % sphere, time=4, value=0)
        cmds.setKeyframe('%s.translateZ' % sphere, time=4, value=3)
        cmds.setKeyframe('%s.rotateX' % sphere, time=4, value=45)
        cmds.setKeyframe('%s.rotateY' % sphere, time=4, value=45)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=4, value=30)
        cmds.setKeyframe('%s.scaleX' % sphere, time=4, value=1)
        cmds.setKeyframe('%s.scaleY' % sphere, time=4, value=3)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=4, value=1)
        
        # Frame 5: 

        cmds.setKeyframe('%s.translateX' % sphere, time=5, value=3)
        cmds.setKeyframe('%s.translateY' % sphere, time=5, value=3)
        cmds.setKeyframe('%s.translateZ' % sphere, time=5, value=3)
        cmds.setKeyframe('%s.rotateX' % sphere, time=5, value=90)
        cmds.setKeyframe('%s.rotateY' % sphere, time=5, value=180)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=5, value=60)
        cmds.setKeyframe('%s.scaleX' % sphere, time=5, value=1)
        cmds.setKeyframe('%s.scaleY' % sphere, time=5, value=1)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=5, value=2.5)
        
        # Frame 6: 
        
        cmds.setKeyframe('%s.translateX' % sphere, time=6, value=0)
        cmds.setKeyframe('%s.translateY' % sphere, time=6, value=0)
        cmds.setKeyframe('%s.translateZ' % sphere, time=6, value=8)
        cmds.setKeyframe('%s.rotateX' % sphere, time=6, value=0)
        cmds.setKeyframe('%s.rotateY' % sphere, time=6, value=0)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=6, value=120)
        cmds.setKeyframe('%s.scaleX' % sphere, time=6, value=0.5)
        cmds.setKeyframe('%s.scaleY' % sphere, time=6, value=0.5)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=6, value=0.5)
        
        
        # Frame 7: 

        cmds.setKeyframe('%s.translateX' % sphere, time=7, value=-4)
        cmds.setKeyframe('%s.translateY' % sphere, time=7, value=6)
        cmds.setKeyframe('%s.translateZ' % sphere, time=7, value=2)
        cmds.setKeyframe('%s.rotateX' % sphere, time=7, value=60)
        cmds.setKeyframe('%s.rotateY' % sphere, time=7, value=270)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=7, value=90)
        cmds.setKeyframe('%s.scaleX' % sphere, time=7, value=0.8)
        cmds.setKeyframe('%s.scaleY' % sphere, time=7, value=2.2)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=7, value=1.3)
        
        # Frame 8:

        cmds.setKeyframe('%s.translateX' % sphere, time=8, value=2)
        cmds.setKeyframe('%s.translateY' % sphere, time=8, value=-3)
        cmds.setKeyframe('%s.translateZ' % sphere, time=8, value=5)
        cmds.setKeyframe('%s.rotateX' % sphere, time=8, value=-45)
        cmds.setKeyframe('%s.rotateY' % sphere, time=8, value=-90)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=8, value=-30)
        cmds.setKeyframe('%s.scaleX' % sphere, time=8, value=3)
        cmds.setKeyframe('%s.scaleY' % sphere, time=8, value=0.3)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=8, value=1.8)
        
        # Frame 9: 
        
        cmds.setKeyframe('%s.translateX' % sphere, time=9, value=1)
        cmds.setKeyframe('%s.translateY' % sphere, time=9, value=1)
        cmds.setKeyframe('%s.translateZ' % sphere, time=9, value=1)
        cmds.setKeyframe('%s.rotateX' % sphere, time=9, value=15)
        cmds.setKeyframe('%s.rotateY' % sphere, time=9, value=15)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=9, value=15)
        cmds.setKeyframe('%s.scaleX' % sphere, time=9, value=1.1)
        cmds.setKeyframe('%s.scaleY' % sphere, time=9, value=1.1)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=9, value=1.1)
        
        # Frame 10: 
        
        cmds.setKeyframe('%s.translateX' % sphere, time=cls.END_TIMECODE, value=0)
        cmds.setKeyframe('%s.translateY' % sphere, time=cls.END_TIMECODE, value=0)
        cmds.setKeyframe('%s.translateZ' % sphere, time=cls.END_TIMECODE, value=0)
        cmds.setKeyframe('%s.rotateX' % sphere, time=cls.END_TIMECODE, value=0)
        cmds.setKeyframe('%s.rotateY' % sphere, time=cls.END_TIMECODE, value=0)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=cls.END_TIMECODE, value=0)
        cmds.setKeyframe('%s.scaleX' % sphere, time=cls.END_TIMECODE, value=1)
        cmds.setKeyframe('%s.scaleY' % sphere, time=cls.END_TIMECODE, value=1)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=cls.END_TIMECODE, value=1)

        # Export to USD
        cls._usdFilePath = os.path.abspath('testUsdExportSplineXforms.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=cls._usdFilePath,
            shadingMode='none',
            animationType='curves',
            frameRange=(cls.START_TIMECODE, cls.END_TIMECODE))

        cls._stage = Usd.Stage.Open(cls._usdFilePath)

    @unittest.skipUnless(Usd.GetVersion() >= (0, 25, 5), 'Splines transforms are only supported in USD 0.25.05 and later')    
    def testExportTransformSplines(self):
        """
        Tests that Maya sphere transform animation exports as USD xform 
        correctly with spline curves for translation, rotation, and scale.
        """

        self.assertTrue(os.path.exists(self._usdFilePath))
        self.assertTrue(self._stage)

        # Test that the sphere exists as a prim
        spherePrimPath = '/animatedSphere'
        spherePrim = self._stage.GetPrimAtPath(spherePrimPath)
        self.assertTrue(spherePrim)
        
        # Test that it is a valid mesh
        mesh = UsdGeom.Mesh(spherePrim)
        self.assertTrue(mesh)
        
        # Get the xformable interface
        sphereXformable = UsdGeom.Xformable(spherePrim)
        self.assertTrue(sphereXformable)

        # Verify transform operations order - should include translation, rotation, and scale
        xformOps = sphereXformable.GetOrderedXformOps()
        self.assertEqual(len(xformOps), 9)
        self.assertEqual(xformOps[0].GetOpName(), "xformOp:translateX")
        self.assertEqual(xformOps[1].GetOpName(), "xformOp:translateY")
        self.assertEqual(xformOps[2].GetOpName(), "xformOp:translateZ")
        self.assertEqual(xformOps[3].GetOpName(), "xformOp:rotateZ")
        self.assertEqual(xformOps[4].GetOpName(), "xformOp:rotateY")
        self.assertEqual(xformOps[5].GetOpName(), "xformOp:rotateX")
        self.assertEqual(xformOps[6].GetOpName(), "xformOp:scaleX")
        self.assertEqual(xformOps[7].GetOpName(), "xformOp:scaleY")
        self.assertEqual(xformOps[8].GetOpName(), "xformOp:scaleZ")
        
        # Test translation splines
        expectedXValues = [
            (1, 0), (2, 5), (3, 5), (4, 0), (5, 3),
            (6, 0), (7, -4), (8, 2), (9, 1), (10, 0)
        ]
        expectedYValues = [
            (1, 0), (2, 0), (3, 5), (4, 0), (5, 3),
            (6, 0), (7, 6), (8, -3), (9, 1), (10, 0)
        ]
        expectedZValues = [
            (1, 0), (2, 0), (3, 0), (4, 3), (5, 3),
            (6, 8), (7, 2), (8, 5), (9, 1), (10, 0)
        ]
        
        self._validateTransformSpline(xformOps[0], expectedXValues)
        self._validateTransformSpline(xformOps[1], expectedYValues)
        self._validateTransformSpline(xformOps[2], expectedZValues)
        
        # Test rotation splines (values in degrees)
        expectedRotXValues = [
            (1, 0), (2, 0), (3, 30), (4, 45), (5, 90),
            (6, 0), (7, 60), (8, -45), (9, 15), (10, 0)
        ]
        expectedRotYValues = [
            (1, 0), (2, 45), (3, 90), (4, 45), (5, 180),
            (6, 0), (7, 270), (8, -90), (9, 15), (10, 0)
        ]
        expectedRotZValues = [
            (1, 0), (2, 0), (3, 0), (4, 30), (5, 60),
            (6, 120), (7, 90), (8, -30), (9, 15), (10, 0)
        ]
        
        self._validateTransformSpline(xformOps[5], expectedRotXValues)
        self._validateTransformSpline(xformOps[4], expectedRotYValues)
        self._validateTransformSpline(xformOps[3], expectedRotZValues)
        
        # Test scale splines
        expectedScaleXValues = [
            (1, 1), (2, 1.5), (3, 2), (4, 1), (5, 1),
            (6, 0.5), (7, 0.8), (8, 3), (9, 1.1), (10, 1)
        ]
        expectedScaleYValues = [
            (1, 1), (2, 1.5), (3, 1), (4, 3), (5, 1),
            (6, 0.5), (7, 2.2), (8, 0.3), (9, 1.1), (10, 1)
        ]
        expectedScaleZValues = [
            (1, 1), (2, 1.5), (3, 1), (4, 1), (5, 2.5),
            (6, 0.5), (7, 1.3), (8, 1.8), (9, 1.1), (10, 1)
        ]
        
        self._validateTransformSpline(xformOps[6], expectedScaleXValues)
        self._validateTransformSpline(xformOps[7], expectedScaleYValues)
        self._validateTransformSpline(xformOps[8], expectedScaleZValues)
        
        # Also test using GetXformVectors for comprehensive transform validation
        timeSamples = [(1, Gf.Vec3d(0, 0, 0), Gf.Vec3f(0, 0, 0), Gf.Vec3f(1, 1, 1)),
                       (2, Gf.Vec3d(5, 0, 0), Gf.Vec3f(0, 45, 0), Gf.Vec3f(1.5, 1.5, 1.5)),
                       (3, Gf.Vec3d(5, 5, 0), Gf.Vec3f(30, 90, 0), Gf.Vec3f(2, 1, 1)),
                       (4, Gf.Vec3d(0, 0, 3), Gf.Vec3f(45, 45, 30), Gf.Vec3f(1, 3, 1)),
                       (5, Gf.Vec3d(3, 3, 3), Gf.Vec3f(90, 180, 60), Gf.Vec3f(1, 1, 2.5)),
                       (6, Gf.Vec3d(0, 0, 8), Gf.Vec3f(0, 0, 120), Gf.Vec3f(0.5, 0.5, 0.5)),
                       (7, Gf.Vec3d(-4, 6, 2), Gf.Vec3f(60, 270, 90), Gf.Vec3f(0.8, 2.2, 1.3)),
                       (8, Gf.Vec3d(2, -3, 5), Gf.Vec3f(-45, -90, -30), Gf.Vec3f(3, 0.3, 1.8)),
                       (9, Gf.Vec3d(1, 1, 1), Gf.Vec3f(15, 15, 15), Gf.Vec3f(1.1, 1.1, 1.1)),
                       (10, Gf.Vec3d(0, 0, 0), Gf.Vec3f(0, 0, 0), Gf.Vec3f(1, 1, 1))]
        
        # Get transform API to evaluate at specific times
        xformAPI = UsdGeom.XformCommonAPI(sphereXformable)
        if xformAPI:
            for time, expectedTranslation, expectedRotationDegrees, expectedScale in timeSamples:
                translation, rotation, scale, pivot, rotOrder = xformAPI.GetXformVectors(time)
                
                # Validate translation
                self.assertTrue(Gf.IsClose(translation, expectedTranslation, self.EPSILON),
                    f"At time {time}, expected translation {expectedTranslation}, got {translation}")
                
                # Validate rotation (convert expected degrees to radians for comparison)
                expectedRotationRadians = Gf.Vec3f(
                    math.radians(expectedRotationDegrees[0]),
                    math.radians(expectedRotationDegrees[1]),
                    math.radians(expectedRotationDegrees[2])
                )
                self.assertTrue(Gf.IsClose(rotation, expectedRotationRadians, self.EPSILON),
                    f"At time {time}, expected rotation {expectedRotationRadians}, got {rotation}")
                
                # Validate scale
                self.assertTrue(Gf.IsClose(scale, expectedScale, self.EPSILON),
                    f"At time {time}, expected scale {expectedScale}, got {scale}")

    def _validateTransformSpline(self, xformOp, expectedValues):
        """
        Helper method to validate transform spline data for a given xform operation.
        
        Args:
            xformOp: The USD xform operation to test
            expectedValues: List of (time, value) tuples for validation
        """
        # Test the attribute exists
        attr = xformOp.GetAttr()
        self.assertTrue(attr)
        
        # Check if spline data exists
        spline = attr.GetSpline()
        if spline:  # Only test splines if USD version supports them
            knots = spline.GetKnots()
            self.assertTrue(len(knots) >= 10)  # Should have 10 keyframes
            
            # Validate specific keyframe values
            for time, expectedValue in expectedValues:
                self.assertTrue(Gf.IsClose(knots[time].GetValue(), expectedValue, self.EPSILON),
                    f"At time {time}, expected {expectedValue}, got {knots[time].GetValue()}")

if __name__ == '__main__':
    unittest.main(verbosity=2)
