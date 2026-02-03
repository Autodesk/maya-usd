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

from pxr import Gf
from pxr import Usd
from pxr import UsdGeom
from pxr import UsdLux
from pxr import UsdRi

from maya import cmds
from maya import standalone
from maya import OpenMaya

import fixturesUtils

class testUsdExportLightAttrSplines(unittest.TestCase):

    START_TIMECODE = 1.0
    END_TIMECODE = 5.0
    EPSILON = 1e-3

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)
        
        # Create a new Maya scene
        cmds.file(new=True, force=True)
        
        # Create directional light
        directionalLight = cmds.directionalLight(name='directionalLight1')
        cmds.setAttr('%s.intensity' % directionalLight, 2)
        cmds.setAttr('%s.color' % directionalLight, 1, 0.9, 0.8, type='double3')
        cmds.setAttr('%s.useDepthMapShadows' % directionalLight, 1)
        cmds.setAttr('%s.shadowColor' % directionalLight, 0.1, 0.2, 0.3, type='double3')
        cmds.setAttr('%s.lightAngle' % directionalLight, 1.5)
        # Animate lightAngle with 4 keyframes: 1.5 -> 1.8 -> 1.2 -> 2
        cmds.setKeyframe('%s.lightAngle' % directionalLight, time=cls.START_TIMECODE, value=1.5)
        cmds.setKeyframe('%s.lightAngle' % directionalLight, time=cls.START_TIMECODE + 1, value=1.8)
        cmds.setKeyframe('%s.lightAngle' % directionalLight, time=cls.START_TIMECODE + 2, value=1.2)
        cmds.setKeyframe('%s.lightAngle' % directionalLight, time=cls.END_TIMECODE, value=2)
        
        # Create point light
        pointLight = cmds.pointLight(name='pointLight1')
        cmds.setAttr('%s.color' % pointLight, 1, 0.5, 0.1, type='double3')
        cmds.setAttr('%s.intensity' % pointLight, 0.5)
        cmds.setAttr('%s.emitSpecular' % pointLight, 0)
        # Animate intensity with 4 keyframes: 0.5 -> 1.2 -> 0.8 -> 2
        cmds.setKeyframe('%s.intensity' % pointLight, time=cls.START_TIMECODE, value=0.5)
        cmds.setKeyframe('%s.intensity' % pointLight, time=cls.START_TIMECODE + 1, value=1.2)
        cmds.setKeyframe('%s.intensity' % pointLight, time=cls.START_TIMECODE + 2, value=0.8)
        cmds.setKeyframe('%s.intensity' % pointLight, time=cls.END_TIMECODE, value=2)
        
        # Create spot light
        spotLight = cmds.spotLight(name='spotLight1')
        cmds.setAttr('%s.color' % spotLight, 0.3, 1, 0.2, type='double3')
        cmds.setAttr('%s.intensity' % spotLight, 0.8)
        cmds.setAttr('%s.emitDiffuse' % spotLight, 0)
        cmds.setAttr('%s.coneAngle' % spotLight, 50)
        cmds.setAttr('%s.penumbraAngle' % spotLight, 20)
        cmds.setAttr('%s.dropoff' % spotLight, 8)
        cmds.setAttr('%s.useDepthMapShadows' % spotLight, 1)
        # Animate color with 4 keyframes for each channel
        # Red: 0.3 -> 0.6 -> 0.1 -> 0
        cmds.setKeyframe('%s.colorR' % spotLight, time=cls.START_TIMECODE, value=0.3)
        cmds.setKeyframe('%s.colorR' % spotLight, time=cls.START_TIMECODE + 1, value=0.6)
        cmds.setKeyframe('%s.colorR' % spotLight, time=cls.START_TIMECODE + 2, value=0.1)
        cmds.setKeyframe('%s.colorR' % spotLight, time=cls.END_TIMECODE, value=0)
        # Green: 1 -> 0.7 -> 0.4 -> 0.2
        cmds.setKeyframe('%s.colorG' % spotLight, time=cls.START_TIMECODE, value=1)
        cmds.setKeyframe('%s.colorG' % spotLight, time=cls.START_TIMECODE + 1, value=0.7)
        cmds.setKeyframe('%s.colorG' % spotLight, time=cls.START_TIMECODE + 2, value=0.4)
        cmds.setKeyframe('%s.colorG' % spotLight, time=cls.END_TIMECODE, value=0.2)
        # Blue: 0.2 -> 0.5 -> 0.3 -> 0.1
        cmds.setKeyframe('%s.colorB' % spotLight, time=cls.START_TIMECODE, value=0.2)
        cmds.setKeyframe('%s.colorB' % spotLight, time=cls.START_TIMECODE + 1, value=0.5)
        cmds.setKeyframe('%s.colorB' % spotLight, time=cls.START_TIMECODE + 2, value=0.3)
        cmds.setKeyframe('%s.colorB' % spotLight, time=cls.END_TIMECODE, value=0.1)
        # Animate coneAngle with 4 keyframes: 50 -> 60 -> 40 -> 50
        cmds.setKeyframe('%s.coneAngle' % spotLight, time=cls.START_TIMECODE, value=50)
        cmds.setKeyframe('%s.coneAngle' % spotLight, time=cls.START_TIMECODE + 1, value=60)
        cmds.setKeyframe('%s.coneAngle' % spotLight, time=cls.START_TIMECODE + 2, value=40)
        cmds.setKeyframe('%s.coneAngle' % spotLight, time=cls.END_TIMECODE, value=50)
        # Animate penumbraAngle with 4 keyframes: 20 -> 25 -> 15 -> 20
        cmds.setKeyframe('%s.penumbraAngle' % spotLight, time=cls.START_TIMECODE, value=20)
        cmds.setKeyframe('%s.penumbraAngle' % spotLight, time=cls.START_TIMECODE + 1, value=25)
        cmds.setKeyframe('%s.penumbraAngle' % spotLight, time=cls.START_TIMECODE + 2, value=15)
        cmds.setKeyframe('%s.penumbraAngle' % spotLight, time=cls.END_TIMECODE, value=20)
        # Animate dropoff with 4 keyframes: 8 -> 10 -> 6 -> 8
        cmds.setKeyframe('%s.dropoff' % spotLight, time=cls.START_TIMECODE, value=8)
        cmds.setKeyframe('%s.dropoff' % spotLight, time=cls.START_TIMECODE + 1, value=10)
        cmds.setKeyframe('%s.dropoff' % spotLight, time=cls.START_TIMECODE + 2, value=6)
        cmds.setKeyframe('%s.dropoff' % spotLight, time=cls.END_TIMECODE, value=8)
        # Animate radius with 4 keyframes: 0 -> 0.5 -> 0.2 -> 0.3
        cmds.setKeyframe('%s.lightRadius' % spotLight, time=cls.START_TIMECODE, value=0)
        cmds.setKeyframe('%s.lightRadius' % spotLight, time=cls.START_TIMECODE + 1, value=0.5)
        cmds.setKeyframe('%s.lightRadius' % spotLight, time=cls.START_TIMECODE + 2, value=0.2)
        cmds.setKeyframe('%s.lightRadius' % spotLight, time=cls.END_TIMECODE, value=0.3)
        
        # Create area light
        cmds.CreateAreaLight(name='areaLight1')
        areaLight = 'areaLight1'
        cmds.setAttr('%s.color' % areaLight, 0.8, 0.7, 0.6, type='double3')
        cmds.setAttr('%s.intensity' % areaLight, 1.2)
        cmds.setAttr('%s.useDepthMapShadows' % areaLight, 1)

        # Export to USD.
        cls._usdFilePath = os.path.abspath('testUsdExportLightAttrSplines.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=cls._usdFilePath,
            shadingMode='none',
            animationType='curves')

        cls._stage = Usd.Stage.Open(cls._usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._stage)

    def _ValidateUsdDistantLight(self):
        lightPrimPath = '/directionalLight1'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)
        distantLight = UsdLux.DistantLight(lightPrim)
        self.assertTrue(distantLight)

        # Validate static attributes
        self.assertTrue(Gf.IsClose(distantLight.GetIntensityAttr().Get(), 2, self.EPSILON))
        self.assertTrue(Gf.IsClose(distantLight.GetColorAttr().Get(), Gf.Vec3f(1, 0.9, 0.8), self.EPSILON))
        self.assertTrue(Gf.IsClose(distantLight.GetDiffuseAttr().Get(), 1, self.EPSILON))
        self.assertTrue(Gf.IsClose(distantLight.GetSpecularAttr().Get(), 1, self.EPSILON))

        # Validate animated angle attribute with spline
        angleAttr = distantLight.GetAngleAttr()
        spline = angleAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()
        self.assertTrue(len(knots) == 4)
        self.assertEqual(knots[1].GetTime(), 1)
        self.assertTrue(Gf.IsClose(knots[1].GetValue(), 1.5, self.EPSILON))
        self.assertEqual(knots[2].GetTime(), 2)
        self.assertTrue(Gf.IsClose(knots[2].GetValue(), 1.8, self.EPSILON))
        self.assertEqual(knots[3].GetTime(), 3)
        self.assertTrue(Gf.IsClose(knots[3].GetValue(), 1.2, self.EPSILON))
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 2, self.EPSILON))

        # Validate that the shadow API is not present
        self.assertFalse(lightPrim.HasAPI(UsdLux.ShadowAPI))

    def _ValidateUsdPointLight(self):
        lightPrimPath = '/pointLight1'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)
        sphereLight = UsdLux.SphereLight(lightPrim)
        self.assertTrue(sphereLight)

        # Validate static attributes
        self.assertTrue(Gf.IsClose(sphereLight.GetColorAttr().Get(), Gf.Vec3f(1, 0.5, 0.1), self.EPSILON))
        self.assertTrue(Gf.IsClose(sphereLight.GetDiffuseAttr().Get(), 1, self.EPSILON))
        self.assertTrue(Gf.IsClose(sphereLight.GetSpecularAttr().Get(), 0, self.EPSILON))
        self.assertTrue(Gf.IsClose(sphereLight.GetRadiusAttr().Get(), 0, self.EPSILON))
        self.assertTrue(sphereLight.GetTreatAsPointAttr().Get())

        # Validate animated intensity attribute with spline
        intensityAttr = sphereLight.GetIntensityAttr()
        spline = intensityAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()
        self.assertTrue(len(knots) == 4)
        self.assertEqual(knots[1].GetTime(), 1)
        self.assertTrue(Gf.IsClose(knots[1].GetValue(), 0.5, self.EPSILON))
        self.assertEqual(knots[2].GetTime(), 2)
        self.assertTrue(Gf.IsClose(knots[2].GetValue(), 1.2, self.EPSILON))
        self.assertEqual(knots[3].GetTime(), 3)
        self.assertTrue(Gf.IsClose(knots[3].GetValue(), 0.8, self.EPSILON))
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 2, self.EPSILON))

        # Validate shadow API
        self.assertTrue(lightPrim.HasAPI(UsdLux.ShadowAPI))
        shadowAPI = UsdLux.ShadowAPI(lightPrim)
        self.assertTrue(shadowAPI)
        self.assertTrue(shadowAPI.GetShadowEnableAttr().Get())

    def _ValidateUsdSpotLight(self):
        lightPrimPath = '/spotLight1'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)
        sphereLight = UsdLux.SphereLight(lightPrim)
        self.assertTrue(sphereLight)

        # Validate static attributes
        self.assertTrue(Gf.IsClose(sphereLight.GetColorAttr().Get(), Gf.Vec3f(0.3, 1, 0.2), self.EPSILON))
        self.assertTrue(Gf.IsClose(sphereLight.GetDiffuseAttr().Get(), 0, self.EPSILON))
        self.assertTrue(Gf.IsClose(sphereLight.GetSpecularAttr().Get(), 1, self.EPSILON))
        self.assertTrue(Gf.IsClose(sphereLight.GetIntensityAttr().Get(), 0.8, self.EPSILON))
        self.assertFalse(sphereLight.GetTreatAsPointAttr().Get())

        # Validate shaping API with animated attributes
        self.assertTrue(lightPrim.HasAPI(UsdLux.ShapingAPI))
        shapingAPI = UsdLux.ShapingAPI(lightPrim)
        self.assertTrue(shapingAPI)

        # Validate animated cone angle (in degrees)
        coneAngleAttr = shapingAPI.GetShapingConeAngleAttr()
        spline = coneAngleAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()
        self.assertTrue(len(knots) == 4)
        self.assertEqual(knots[1].GetTime(), 1)
        self.assertTrue(Gf.IsClose(knots[1].GetValue(), 45, self.EPSILON))
        self.assertEqual(knots[2].GetTime(), 2)
        self.assertTrue(Gf.IsClose(knots[2].GetValue(), 55, self.EPSILON))
        self.assertEqual(knots[3].GetTime(), 3)
        self.assertTrue(Gf.IsClose(knots[3].GetValue(), 35, self.EPSILON))
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 45, self.EPSILON))

        # Validate animated cone softness (converted from Maya's penumbra angle)
        coneSoftnessAttr = shapingAPI.GetShapingConeSoftnessAttr()
        spline = coneSoftnessAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()
        self.assertTrue(len(knots) == 4)
        self.assertEqual(knots[1].GetTime(), 1)
        self.assertTrue(Gf.IsClose(knots[1].GetValue(), 0.44444445, self.EPSILON))
        self.assertEqual(knots[2].GetTime(), 2)
        self.assertTrue(Gf.IsClose(knots[2].GetValue(), 0.45454547, self.EPSILON))
        self.assertEqual(knots[3].GetTime(), 3)
        self.assertTrue(Gf.IsClose(knots[3].GetValue(), 0.42857143, self.EPSILON))
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 0.44444445, self.EPSILON))

        # Validate animated focus (converted from Maya's dropoff)
        focusAttr = shapingAPI.GetShapingFocusAttr()
        spline = focusAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()
        self.assertTrue(len(knots) == 4)
        self.assertEqual(knots[1].GetTime(), 1)
        self.assertEqual(knots[1].GetValue(), 8)
        self.assertEqual(knots[2].GetTime(), 2)
        self.assertEqual(knots[2].GetValue(), 10)
        self.assertEqual(knots[3].GetTime(), 3)
        self.assertEqual(knots[3].GetValue(), 6)
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertEqual(knots[5].GetValue(), 8)

        # Validate animated radius attribute
        radiusAttr = sphereLight.GetRadiusAttr()
        spline = radiusAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()
        self.assertTrue(len(knots) == 4)
        self.assertEqual(knots[1].GetTime(), 1)
        self.assertTrue(Gf.IsClose(knots[1].GetValue(), 0, self.EPSILON))
        self.assertEqual(knots[2].GetTime(), 2)
        self.assertTrue(Gf.IsClose(knots[2].GetValue(), 0.5, self.EPSILON))
        self.assertEqual(knots[3].GetTime(), 3)
        self.assertTrue(Gf.IsClose(knots[3].GetValue(), 0.2, self.EPSILON))
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 0.3, self.EPSILON))

    def _ValidateUsdAreaLight(self):
        lightPrimPath = '/areaLight1'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)
        rectLight = UsdLux.RectLight(lightPrim)
        self.assertTrue(rectLight)

        # Validate static attributes
        self.assertTrue(Gf.IsClose(rectLight.GetColorAttr().Get(), Gf.Vec3f(0.8, 0.7, 0.6), self.EPSILON))
        self.assertTrue(Gf.IsClose(rectLight.GetIntensityAttr().Get(), 1.2, self.EPSILON))
        self.assertTrue(Gf.IsClose(rectLight.GetDiffuseAttr().Get(), 1, self.EPSILON))
        self.assertTrue(Gf.IsClose(rectLight.GetSpecularAttr().Get(), 1, self.EPSILON))
        
        # Validate normalize attribute
        self.assertTrue(rectLight.GetNormalizeAttr().Get())

    def _GetMayaDependencyNode(self, objectName):
        selectionList = OpenMaya.MSelectionList()
        selectionList.add(objectName)
        mObj = OpenMaya.MObject()
        selectionList.getDependNode(0, mObj)
        depNodeFn = OpenMaya.MFnDependencyNode(mObj)
        self.assertTrue(depNodeFn)

        return depNodeFn

    def _ValidateMayaDistantLight(self):
        nodePath = 'directionalLight1'        
        depNodeFn = self._GetMayaDependencyNode(nodePath)
        self.assertTrue(depNodeFn)
        
        # Validate static attributes
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.intensity' % nodePath), 2, self.EPSILON))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorR' % nodePath), 1, self.EPSILON))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorG' % nodePath), 0.9, self.EPSILON))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorB' % nodePath), 0.8, self.EPSILON))
        
        # Validate animated lightAngle at different time codes
        cmds.currentTime(1)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.lightAngle' % nodePath), 1.5, self.EPSILON))
        cmds.currentTime(2)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.lightAngle' % nodePath), 1.8, self.EPSILON))
        cmds.currentTime(3)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.lightAngle' % nodePath), 1.2, self.EPSILON))
        cmds.currentTime(5)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.lightAngle' % nodePath), 2, self.EPSILON))

    def _ValidateMayaPointLight(self):
        nodePath = 'pointLight1'
        depNodeFn = self._GetMayaDependencyNode(nodePath)
        self.assertTrue(depNodeFn)
        
        # Validate static attributes
        self.assertTrue(cmds.getAttr('%s.emitDiffuse' % nodePath) == 1)
        self.assertTrue(cmds.getAttr('%s.emitSpecular' % nodePath) == 0)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorR' % nodePath), 1, self.EPSILON))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorG' % nodePath), 0.5, self.EPSILON))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorB' % nodePath), 0.1, self.EPSILON))
        
        # Validate animated intensity at different time codes
        cmds.currentTime(1)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.intensity' % nodePath), 0.5, self.EPSILON))
        cmds.currentTime(2)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.intensity' % nodePath), 1.2, self.EPSILON))
        cmds.currentTime(3)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.intensity' % nodePath), 0.8, self.EPSILON))
        cmds.currentTime(5)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.intensity' % nodePath), 2, self.EPSILON))

    def _ValidateMayaSpotLight(self):
        nodePath = 'spotLight1'
        depNodeFn = self._GetMayaDependencyNode(nodePath)
        self.assertTrue(depNodeFn)
        
        # Validate static attributes
        self.assertTrue(cmds.getAttr('%s.emitDiffuse' % nodePath) == 0)
        self.assertTrue(cmds.getAttr('%s.emitSpecular' % nodePath) == 1)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.intensity' % nodePath), 0.8, self.EPSILON))
        
        # Validate static color (not animated in USD export)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorR' % nodePath), 0.3, self.EPSILON))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorG' % nodePath), 1, self.EPSILON))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorB' % nodePath), 0.2, self.EPSILON))
        
        # Validate animated coneAngle at different time codes
        cmds.currentTime(1)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.coneAngle' % nodePath), 50, self.EPSILON))
        cmds.currentTime(2)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.coneAngle' % nodePath), 60, self.EPSILON))
        cmds.currentTime(3)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.coneAngle' % nodePath), 40, self.EPSILON))
        cmds.currentTime(5)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.coneAngle' % nodePath), 50, self.EPSILON))
        
        # Validate animated penumbraAngle at different time codes
        cmds.currentTime(1)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.penumbraAngle' % nodePath), 20, self.EPSILON))
        cmds.currentTime(2)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.penumbraAngle' % nodePath), 25, self.EPSILON))
        cmds.currentTime(3)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.penumbraAngle' % nodePath), 15, self.EPSILON))
        cmds.currentTime(5)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.penumbraAngle' % nodePath), 20, self.EPSILON))
        
        # Validate animated dropoff at different time codes
        cmds.currentTime(1)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.dropoff' % nodePath), 8, self.EPSILON))
        cmds.currentTime(2)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.dropoff' % nodePath), 10, self.EPSILON))
        cmds.currentTime(3)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.dropoff' % nodePath), 6, self.EPSILON))
        cmds.currentTime(5)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.dropoff' % nodePath), 8, self.EPSILON))
        
        # Validate animated lightRadius at different time codes
        cmds.currentTime(1)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.lightRadius' % nodePath), 0, self.EPSILON))
        cmds.currentTime(2)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.lightRadius' % nodePath), 0.5, self.EPSILON))
        cmds.currentTime(3)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.lightRadius' % nodePath), 0.2, self.EPSILON))
        cmds.currentTime(5)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.lightRadius' % nodePath), 0.3, self.EPSILON))

    def _ValidateMayaAreaLight(self):
        nodePath = 'areaLight1'
        depNodeFn = self._GetMayaDependencyNode(nodePath)
        self.assertTrue(depNodeFn)
        
        # Validate static attributes
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorR' % nodePath), 0.8, self.EPSILON))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorG' % nodePath), 0.7, self.EPSILON))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorB' % nodePath), 0.6, self.EPSILON))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.intensity' % nodePath), 1.2, self.EPSILON))
        
        # Validate normalize attribute
        self.assertTrue(cmds.getAttr('%s.normalize' % nodePath) == 1)

    @unittest.skipUnless(Usd.GetVersion() >= (0, 24, 11), 'Splines are only supported in USD 0.24.11 and later')    
    def testExportLightsAttrSplines(self):
        """
        Tests that Maya lights export as UsdLux schema USD prims
        correctly.
        """
        self._ValidateUsdDistantLight()
        self._ValidateUsdPointLight()
        self._ValidateUsdSpotLight()
        self._ValidateUsdAreaLight()

    @unittest.skipUnless(Usd.GetVersion() >= (0, 24, 11), 'Splines are only supported in USD 0.24.11 and later')    
    def testImportLightsAttrSplines(self):
        """
        Tests that USD lights with spline animation import back to Maya
        with correct attribute values.
        """
        # Clear Maya scene
        cmds.file(new=True, force=True)
        
        # Import from USD
        cmds.usdImport(file=self._usdFilePath, readAnimData=True, primPath='/')
        
        # Validate Maya lights have correct attribute values
        self._ValidateMayaDistantLight()
        self._ValidateMayaPointLight()
        self._ValidateMayaSpotLight()
        self._ValidateMayaAreaLight()
        
        # Reset time to frame 1
        cmds.currentTime(1)

if __name__ == '__main__':
    unittest.main(verbosity=2)
