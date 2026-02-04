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
import math

from pxr import Usd, Gf, UsdGeom

from maya import cmds
from maya.api import OpenMaya as OM
from maya import standalone

import fixturesUtils

class testUsdImportXformAnim(unittest.TestCase):

    START_TIMECODE = 1.0
    END_TIMECODE = 5.0
    EPSILON = 1e-3

    def _LoadUsd(self):
        # Import the USD file.
        usdFilePath = os.path.join(self.inputPath, "UsdImportXformAnimTest", "Mesh.usda")
        cmds.usdImport(file=usdFilePath, readAnimData=False, upAxis=False)
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
        self.assertTrue(spline)

        knots = spline.GetKnots()
        self.assertTrue(len(knots) >= 5)  # Should have 5 keyframes
            
        # Validate specific keyframe values
        for time, expectedValue in expectedValues:
            self.assertTrue(Gf.IsClose(knots[time].GetValue(), expectedValue, self.EPSILON),
                f"At time {time}, expected {expectedValue}, got {knots[time].GetValue()}")

    def _validateMayaTransformAnimation(self, objectName, transformType, expectedValues):
        """
        Helper method to validate Maya transform animation for a given attribute type.
        
        Args:
            objectName: Name of the Maya object to test
            transformType: Transform type ('translate', 'rotate', 'scale')
            expectedValues: List of (time, (X, Y, Z)) tuples for validation
        """
        for time, (expectedX, expectedY, expectedZ) in expectedValues:
            cmds.currentTime(time)
            actualX = cmds.getAttr(f'{objectName}.{transformType}X')
            actualY = cmds.getAttr(f'{objectName}.{transformType}Y')
            actualZ = cmds.getAttr(f'{objectName}.{transformType}Z')
            
            self.assertTrue(abs(actualX - expectedX) < self.EPSILON,
                f"At time {time}, expected {transformType}X {expectedX}, got {actualX}")
            self.assertTrue(abs(actualY - expectedY) < self.EPSILON,
                f"At time {time}, expected {transformType}Y {expectedY}, got {actualY}")
            self.assertTrue(abs(actualZ - expectedZ) < self.EPSILON,
                f"At time {time}, expected {transformType}Z {expectedZ}, got {actualZ}")

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

    @unittest.skipUnless(Usd.GetVersion() >= (0, 25, 5), 'Splines transforms are only supported in USD 0.25.05 and later')    
    def testUsdImportXformSplineAnim(self):
        """
        Tests that the xform spline anim is imported correctly.
        """
        # Create a Maya scene with animated sphere transform
        sphereResult = cmds.polySphere(name='animatedSphere', radius=1)
        sphere = sphereResult[0]  # Get the transform node
        
        # Set keyframes for translation
        cmds.setKeyframe('%s.translateX' % sphere, time=self.START_TIMECODE, value=0)
        cmds.setKeyframe('%s.translateY' % sphere, time=self.START_TIMECODE, value=0)
        cmds.setKeyframe('%s.translateZ' % sphere, time=self.START_TIMECODE, value=0)
        
        cmds.setKeyframe('%s.translateX' % sphere, time=2, value=5)
        cmds.setKeyframe('%s.translateY' % sphere, time=2, value=2)
        cmds.setKeyframe('%s.translateZ' % sphere, time=2, value=1)
        
        cmds.setKeyframe('%s.translateX' % sphere, time=3, value=3)
        cmds.setKeyframe('%s.translateY' % sphere, time=3, value=4)
        cmds.setKeyframe('%s.translateZ' % sphere, time=3, value=2)
        
        cmds.setKeyframe('%s.translateX' % sphere, time=4, value=1)
        cmds.setKeyframe('%s.translateY' % sphere, time=4, value=1)
        cmds.setKeyframe('%s.translateZ' % sphere, time=4, value=3)
        
        cmds.setKeyframe('%s.translateX' % sphere, time=self.END_TIMECODE, value=0)
        cmds.setKeyframe('%s.translateY' % sphere, time=self.END_TIMECODE, value=0)
        cmds.setKeyframe('%s.translateZ' % sphere, time=self.END_TIMECODE, value=0)
        
        # Set keyframes for rotation
        cmds.setKeyframe('%s.rotateX' % sphere, time=self.START_TIMECODE, value=0)
        cmds.setKeyframe('%s.rotateY' % sphere, time=self.START_TIMECODE, value=0)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=self.START_TIMECODE, value=0)
        
        cmds.setKeyframe('%s.rotateX' % sphere, time=2, value=30)
        cmds.setKeyframe('%s.rotateY' % sphere, time=2, value=45)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=2, value=15)
        
        cmds.setKeyframe('%s.rotateX' % sphere, time=3, value=60)
        cmds.setKeyframe('%s.rotateY' % sphere, time=3, value=90)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=3, value=30)
        
        cmds.setKeyframe('%s.rotateX' % sphere, time=4, value=45)
        cmds.setKeyframe('%s.rotateY' % sphere, time=4, value=120)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=4, value=60)
        
        cmds.setKeyframe('%s.rotateX' % sphere, time=self.END_TIMECODE, value=0)
        cmds.setKeyframe('%s.rotateY' % sphere, time=self.END_TIMECODE, value=0)
        cmds.setKeyframe('%s.rotateZ' % sphere, time=self.END_TIMECODE, value=0)
        
        # Set keyframes for scale
        cmds.setKeyframe('%s.scaleX' % sphere, time=self.START_TIMECODE, value=1)
        cmds.setKeyframe('%s.scaleY' % sphere, time=self.START_TIMECODE, value=1)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=self.START_TIMECODE, value=1)
        
        cmds.setKeyframe('%s.scaleX' % sphere, time=2, value=1.5)
        cmds.setKeyframe('%s.scaleY' % sphere, time=2, value=1.2)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=2, value=1.8)
        
        cmds.setKeyframe('%s.scaleX' % sphere, time=3, value=2)
        cmds.setKeyframe('%s.scaleY' % sphere, time=3, value=0.8)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=3, value=1.3)
        
        cmds.setKeyframe('%s.scaleX' % sphere, time=4, value=0.7)
        cmds.setKeyframe('%s.scaleY' % sphere, time=4, value=2.2)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=4, value=0.9)
        
        cmds.setKeyframe('%s.scaleX' % sphere, time=self.END_TIMECODE, value=1)
        cmds.setKeyframe('%s.scaleY' % sphere, time=self.END_TIMECODE, value=1)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=self.END_TIMECODE, value=1)

        # Export to USD with spline curves
        usdFilePath = os.path.abspath('testUsdImportXformSplineAnim.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none',
            animationType='curves',
            frameRange=(self.START_TIMECODE, self.END_TIMECODE))

        # Validate that USD file was created and contains spline data
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        # Test that the sphere exists as a prim
        spherePrimPath = '/animatedSphere'
        spherePrim = stage.GetPrimAtPath(spherePrimPath)
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
        expectedTranslateXValues = [(1, 0), (2, 5), (3, 3), (4, 1), (5, 0)]
        expectedTranslateYValues = [(1, 0), (2, 2), (3, 4), (4, 1), (5, 0)]
        expectedTranslateZValues = [(1, 0), (2, 1), (3, 2), (4, 3), (5, 0)]
        
        self._validateTransformSpline(xformOps[0], expectedTranslateXValues)
        self._validateTransformSpline(xformOps[1], expectedTranslateYValues)
        self._validateTransformSpline(xformOps[2], expectedTranslateZValues)
        
        # Test rotation splines (values in degrees)
        expectedRotateXValues = [(1, 0), (2, 30), (3, 60), (4, 45), (5, 0)]
        expectedRotateYValues = [(1, 0), (2, 45), (3, 90), (4, 120), (5, 0)]
        expectedRotateZValues = [(1, 0), (2, 15), (3, 30), (4, 60), (5, 0)]
        
        self._validateTransformSpline(xformOps[5], expectedRotateXValues)
        self._validateTransformSpline(xformOps[4], expectedRotateYValues)
        self._validateTransformSpline(xformOps[3], expectedRotateZValues)
        
        # Test scale splines
        expectedScaleXValues = [(1, 1), (2, 1.5), (3, 2), (4, 0.7), (5, 1)]
        expectedScaleYValues = [(1, 1), (2, 1.2), (3, 0.8), (4, 2.2), (5, 1)]
        expectedScaleZValues = [(1, 1), (2, 1.8), (3, 1.3), (4, 0.9), (5, 1)]
        
        self._validateTransformSpline(xformOps[6], expectedScaleXValues)
        self._validateTransformSpline(xformOps[7], expectedScaleYValues)
        self._validateTransformSpline(xformOps[8], expectedScaleZValues)
        
        # Clear Maya scene and import the USD file
        cmds.file(new=True, force=True)
        
        # Import from USD with animation data
        cmds.usdImport(file=usdFilePath, readAnimData=True, primPath='/')
        
        # Validate imported sphere exists
        self.assertTrue(cmds.objExists('animatedSphere'))
        
        # Expected animation values for validation
        expectedTranslationValues = [
            (1, (0, 0, 0)),
            (2, (5, 2, 1)),
            (3, (3, 4, 2)),
            (4, (1, 1, 3)),
            (5, (0, 0, 0))
        ]
        
        expectedRotationValues = [
            (1, (0, 0, 0)),
            (2, (30, 45, 15)),
            (3, (60, 90, 30)),
            (4, (45, 120, 60)),
            (5, (0, 0, 0))
        ]
        
        expectedScaleValues = [
            (1, (1, 1, 1)),
            (2, (1.5, 1.2, 1.8)),
            (3, (2, 0.8, 1.3)),
            (4, (0.7, 2.2, 0.9)),
            (5, (1, 1, 1))
        ]
        
        # Validate all transform animations using helper method
        self._validateMayaTransformAnimation('animatedSphere', 'translate', expectedTranslationValues)
        self._validateMayaTransformAnimation('animatedSphere', 'rotate', expectedRotationValues)
        self._validateMayaTransformAnimation('animatedSphere', 'scale', expectedScaleValues)

    @unittest.skipUnless(Usd.GetVersion() >= (0, 25, 5), 'Splines transforms are only supported in USD 0.25.05 and later')    
    def testUsdImportXformSingleSplineAnim(self):
        """
        Tests that single-axis xform spline animation is imported correctly.
        This test animates only one axis per transform type (translateX, rotateY, scaleZ)
        while setting static non-default values for the non-animated axes.
        """
        # Create a Maya scene with animated sphere transform - single axis per transform type
        sphereResult = cmds.polySphere(name='singleAxisSphere', radius=1)
        sphere = sphereResult[0]  # Get the transform node
        
        # Set keyframes for translation X only (animated)
        cmds.setKeyframe('%s.translateX' % sphere, time=self.START_TIMECODE, value=0)
        cmds.setKeyframe('%s.translateX' % sphere, time=2, value=3)
        cmds.setKeyframe('%s.translateX' % sphere, time=3, value=6)
        cmds.setKeyframe('%s.translateX' % sphere, time=4, value=2)
        cmds.setKeyframe('%s.translateX' % sphere, time=self.END_TIMECODE, value=0)
        
        # Set static values for non-animated translation axes
        cmds.setAttr('%s.translateY' % sphere, 2.5)
        cmds.setAttr('%s.translateZ' % sphere, -1.5)
        
        # Set keyframes for rotation Y only (animated)
        cmds.setKeyframe('%s.rotateY' % sphere, time=self.START_TIMECODE, value=0)
        cmds.setKeyframe('%s.rotateY' % sphere, time=2, value=60)
        cmds.setKeyframe('%s.rotateY' % sphere, time=3, value=120)
        cmds.setKeyframe('%s.rotateY' % sphere, time=4, value=180)
        cmds.setKeyframe('%s.rotateY' % sphere, time=self.END_TIMECODE, value=0)
        
        # Set static values for non-animated rotation axes
        cmds.setAttr('%s.rotateX' % sphere, 15)
        cmds.setAttr('%s.rotateZ' % sphere, -30)
        
        # Set keyframes for scale Z only (animated)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=self.START_TIMECODE, value=1)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=2, value=2)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=3, value=0.5)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=4, value=3)
        cmds.setKeyframe('%s.scaleZ' % sphere, time=self.END_TIMECODE, value=1)
        
        # Set static values for non-animated scale axes
        cmds.setAttr('%s.scaleX' % sphere, 1.2)
        cmds.setAttr('%s.scaleY' % sphere, 0.8)

        # Export to USD with spline curves
        usdFilePath = os.path.abspath('testUsdImportXformSingleSplineAnim.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none',
            animationType='curves',
            frameRange=(self.START_TIMECODE, self.END_TIMECODE))

        # Validate that USD file was created and contains spline data
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        # Test that the sphere exists as a prim
        spherePrimPath = '/singleAxisSphere'
        spherePrim = stage.GetPrimAtPath(spherePrimPath)
        self.assertTrue(spherePrim)
        
        # Test that it is a valid mesh
        mesh = UsdGeom.Mesh(spherePrim)
        self.assertTrue(mesh)
        
        # Get the xformable interface
        sphereXformable = UsdGeom.Xformable(spherePrim)
        self.assertTrue(sphereXformable)

        # Verify transform operations - should have individual axis transforms
        xformOps = sphereXformable.GetOrderedXformOps()
        self.assertTrue(len(xformOps) >= 3)  # At least translateX, rotateY, scaleZ
        
        # Find the specific single-axis operations
        translateXOp = None
        rotateYOp = None  
        scaleZOp = None
        
        for op in xformOps:
            opName = op.GetOpName()
            if opName == "xformOp:translateX":
                translateXOp = op
            elif opName == "xformOp:rotateY":
                rotateYOp = op
            elif opName == "xformOp:scaleZ":
                scaleZOp = op
        
        # Validate that we found the expected operations
        self.assertIsNotNone(translateXOp, "translateX operation not found")
        self.assertIsNotNone(rotateYOp, "rotateY operation not found") 
        self.assertIsNotNone(scaleZOp, "scaleZ operation not found")
        
        # Test translation X spline
        expectedTranslateXValues = [(1, 0), (2, 3), (3, 6), (4, 2), (5, 0)]
        self._validateTransformSpline(translateXOp, expectedTranslateXValues)
        
        # Test rotation Y spline (values in degrees)
        expectedRotateYValues = [(1, 0), (2, 60), (3, 120), (4, 180), (5, 0)]
        self._validateTransformSpline(rotateYOp, expectedRotateYValues)
        
        # Test scale Z spline
        expectedScaleZValues = [(1, 1), (2, 2), (3, 0.5), (4, 3), (5, 1)]
        self._validateTransformSpline(scaleZOp, expectedScaleZValues)
        
        # Clear Maya scene and import the USD file
        cmds.file(new=True, force=True)
        
        # Import from USD with animation data
        cmds.usdImport(file=usdFilePath, readAnimData=True, primPath='/')
        
        # Validate imported sphere exists
        self.assertTrue(cmds.objExists('singleAxisSphere'))
        
        # Expected animation values for validation - X animated, Y/Z static
        expectedTranslationValues = [
            (1, (0, 2.5, -1.5)),    # X animated, Y/Z static
            (2, (3, 2.5, -1.5)),
            (3, (6, 2.5, -1.5)),
            (4, (2, 2.5, -1.5)),
            (5, (0, 2.5, -1.5))
        ]
        
        expectedRotationValues = [
            (1, (15, 0, -30)),    # Y animated, X/Z static
            (2, (15, 60, -30)),
            (3, (15, 120, -30)),
            (4, (15, 180, -30)),
            (5, (15, 0, -30))
        ]
        
        expectedScaleValues = [
            (1, (1.2, 0.8, 1)),    # Z animated, X/Y static
            (2, (1.2, 0.8, 2)),
            (3, (1.2, 0.8, 0.5)),
            (4, (1.2, 0.8, 3)),
            (5, (1.2, 0.8, 1))
        ]
        
        # Validate single-axis transform animations using helper method
        self._validateMayaTransformAnimation('singleAxisSphere', 'translate', expectedTranslationValues)
        self._validateMayaTransformAnimation('singleAxisSphere', 'rotate', expectedRotationValues)
        self._validateMayaTransformAnimation('singleAxisSphere', 'scale', expectedScaleValues)
        

if __name__ == '__main__':
    unittest.main(verbosity=2)
