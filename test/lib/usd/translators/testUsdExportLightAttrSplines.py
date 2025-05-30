#!/usr/bin/env mayapy
#
# Copyright 2021 Autodesk
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
        cmds.setAttr('%s.coneAngle' % spotLight, 50)  # This will be converted to half-angle (25) in USD
        cmds.setAttr('%s.penumbraAngle' % spotLight, 20)  # This affects cone softness
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

    def _ValidateDistantLight(self):
        lightPrimPath = '/directionalLight1'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)
        distantLight = UsdLux.DistantLight(lightPrim)
        self.assertTrue(distantLight)

        # Validate static attributes
        self.assertTrue(Gf.IsClose(distantLight.GetIntensityAttr().Get(), 2, 1e-6))
        self.assertTrue(Gf.IsClose(distantLight.GetColorAttr().Get(), Gf.Vec3f(1, 0.9, 0.8), 1e-6))
        self.assertTrue(Gf.IsClose(distantLight.GetDiffuseAttr().Get(), 1, 1e-6))
        self.assertTrue(Gf.IsClose(distantLight.GetSpecularAttr().Get(), 1, 1e-6))

        # Validate animated angle attribute with spline
        angleAttr = distantLight.GetAngleAttr()
        spline = angleAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()
        self.assertTrue(len(knots) == 4)
        self.assertEqual(knots[1].GetTime(), 1)
        self.assertTrue(Gf.IsClose(knots[1].GetValue(), 1.5, 1e-6))
        self.assertEqual(knots[2].GetTime(), 2)
        self.assertTrue(Gf.IsClose(knots[2].GetValue(), 1.8, 1e-6))
        self.assertEqual(knots[3].GetTime(), 3)
        self.assertTrue(Gf.IsClose(knots[3].GetValue(), 1.2, 1e-6))
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 2, 1e-6))

        # Validate shadow API
        self.assertTrue(lightPrim.HasAPI(UsdLux.ShadowAPI))
        shadowAPI = UsdLux.ShadowAPI(lightPrim)
        self.assertTrue(shadowAPI)
        self.assertFalse(shadowAPI.GetShadowEnableAttr().Get())
        self.assertTrue(Gf.IsClose(shadowAPI.GetShadowColorAttr().Get(), Gf.Vec3f(0.1, 0.2, 0.3), 1e-6))

    def _ValidatePointLight(self):
        lightPrimPath = '/pointLight1'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)
        sphereLight = UsdLux.SphereLight(lightPrim)
        self.assertTrue(sphereLight)

        # Validate static attributes
        self.assertTrue(Gf.IsClose(sphereLight.GetColorAttr().Get(), Gf.Vec3f(1, 0.5, 0.1), 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetDiffuseAttr().Get(), 1, 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetSpecularAttr().Get(), 0, 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetRadiusAttr().Get(), 0, 1e-6))
        self.assertTrue(sphereLight.GetTreatAsPointAttr().Get())

        # Validate animated intensity attribute with spline
        intensityAttr = sphereLight.GetIntensityAttr()
        spline = intensityAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()
        self.assertTrue(len(knots) == 4)
        self.assertEqual(knots[1].GetTime(), 1)
        self.assertTrue(Gf.IsClose(knots[1].GetValue(), 0.5, 1e-6))
        self.assertEqual(knots[2].GetTime(), 2)
        self.assertTrue(Gf.IsClose(knots[2].GetValue(), 1.2, 1e-6))
        self.assertEqual(knots[3].GetTime(), 3)
        self.assertTrue(Gf.IsClose(knots[3].GetValue(), 0.8, 1e-6))
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 2, 1e-6))

        # Validate shadow API
        self.assertTrue(lightPrim.HasAPI(UsdLux.ShadowAPI))
        shadowAPI = UsdLux.ShadowAPI(lightPrim)
        self.assertTrue(shadowAPI)
        self.assertTrue(shadowAPI.GetShadowEnableAttr().Get())

    def _ValidateSpotLight(self):
        lightPrimPath = '/spotLight1'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)
        sphereLight = UsdLux.SphereLight(lightPrim)
        self.assertTrue(sphereLight)

        # Validate static attributes
        self.assertTrue(Gf.IsClose(sphereLight.GetColorAttr().Get(), Gf.Vec3f(0.3, 1, 0.2), 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetDiffuseAttr().Get(), 0, 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetSpecularAttr().Get(), 1, 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetIntensityAttr().Get(), 0.8, 1e-6))
        self.assertFalse(sphereLight.GetTreatAsPointAttr().Get())

        # Validate shadow API
        self.assertTrue(lightPrim.HasAPI(UsdLux.ShadowAPI))
        shadowAPI = UsdLux.ShadowAPI(lightPrim)
        self.assertTrue(shadowAPI)
        self.assertFalse(shadowAPI.GetShadowEnableAttr().Get())

        # Validate shaping API with animated attributes
        self.assertTrue(lightPrim.HasAPI(UsdLux.ShapingAPI))
        shapingAPI = UsdLux.ShapingAPI(lightPrim)
        self.assertTrue(shapingAPI)

        # Validate animated cone angle (converted from Maya's full cone angle to USD's half angle)
        coneAngleAttr = shapingAPI.GetShapingConeAngleAttr()
        spline = coneAngleAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()
        self.assertTrue(len(knots) == 4)
        # Maya 50° -> USD ~0.349 radians (25°), Maya 60° -> USD ~0.436 radians (30°), etc.
        self.assertEqual(knots[1].GetTime(), 1)
        self.assertTrue(Gf.IsClose(knots[1].GetValue(), 0.34906584, 1e-6))
        self.assertEqual(knots[2].GetTime(), 2)
        self.assertTrue(Gf.IsClose(knots[2].GetValue(), 0.43633232, 1e-6))
        self.assertEqual(knots[3].GetTime(), 3)
        self.assertTrue(Gf.IsClose(knots[3].GetValue(), 0.2617994, 1e-6))
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 0.34906584, 1e-6))

        # Validate animated cone softness (converted from Maya's penumbra angle)
        coneSoftnessAttr = shapingAPI.GetShapingConeSoftnessAttr()
        spline = coneSoftnessAttr.GetSpline()
        self.assertTrue(spline)
        knots = spline.GetKnots()
        self.assertTrue(len(knots) == 4)
        self.assertEqual(knots[1].GetTime(), 1)
        self.assertTrue(Gf.IsClose(knots[1].GetValue(), 0.34906584, 1e-6))
        self.assertEqual(knots[2].GetTime(), 2)
        self.assertTrue(Gf.IsClose(knots[2].GetValue(), 0.43633232, 1e-6))
        self.assertEqual(knots[3].GetTime(), 3)
        self.assertTrue(Gf.IsClose(knots[3].GetValue(), 0.2617994, 1e-6))
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 0.34906584, 1e-6))

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
        self.assertTrue(Gf.IsClose(knots[1].GetValue(), 0, 1e-6))
        self.assertEqual(knots[2].GetTime(), 2)
        self.assertTrue(Gf.IsClose(knots[2].GetValue(), 0.5, 1e-6))
        self.assertEqual(knots[3].GetTime(), 3)
        self.assertTrue(Gf.IsClose(knots[3].GetValue(), 0.2, 1e-6))
        self.assertEqual(knots[5].GetTime(), 5)
        self.assertTrue(Gf.IsClose(knots[5].GetValue(), 0.3, 1e-6))

    def _ValidateAreaLight(self):
        lightPrimPath = '/areaLight1'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)
        rectLight = UsdLux.RectLight(lightPrim)
        self.assertTrue(rectLight)

        # Validate static attributes
        self.assertTrue(Gf.IsClose(rectLight.GetColorAttr().Get(), Gf.Vec3f(0.8, 0.7, 0.6), 1e-6))
        self.assertTrue(Gf.IsClose(rectLight.GetIntensityAttr().Get(), 1.2, 1e-6))
        self.assertTrue(Gf.IsClose(rectLight.GetDiffuseAttr().Get(), 1, 1e-6))
        self.assertTrue(Gf.IsClose(rectLight.GetSpecularAttr().Get(), 1, 1e-6))
        
        # Validate normalize attribute
        self.assertTrue(rectLight.GetNormalizeAttr().Get())

        # Validate shadow API
        self.assertTrue(lightPrim.HasAPI(UsdLux.ShadowAPI))
        shadowAPI = UsdLux.ShadowAPI(lightPrim)
        self.assertTrue(shadowAPI)
        self.assertFalse(shadowAPI.GetShadowEnableAttr().Get())

    @unittest.skipUnless(Usd.GetVersion() >= (0, 24, 11), 'Splines are only supported in USD 0.24.11 and later')    
    def testExportLightsAttrSplines(self):
        """
        Tests that Maya lights export as UsdLux schema USD prims
        correctly.
        """
        self._ValidateDistantLight()
        self._ValidatePointLight()
        self._ValidateSpotLight()
        self._ValidateAreaLight()

if __name__ == '__main__':
    unittest.main(verbosity=2)
