#!/usr/bin/env mayapy
#
# Copyright 2017 Pixar
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
from pxr import Tf
from pxr import Usd
from pxr import UsdGeom
from pxr import UsdLux
from pxr import UsdRi

from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdExportRfMLight(unittest.TestCase):

    START_TIMECODE = 1.0
    END_TIMECODE = 5.0

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        mayaFile = os.path.join(inputPath, 'UsdExportRfMLightTest', 'RfMLightsTest.ma')
        cmds.file(mayaFile, open=True, force=True)

        # Export to USD.
        usdFilePath = os.path.abspath('RfMLightsTest.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='pxrRis',
            frameRange=(cls.START_TIMECODE, cls.END_TIMECODE))

        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _assertGfIsClose(self, a, b, ep):
        if Gf.IsClose(a, b, ep):
            return
        raise self.failureException("%s != %s (ep=%f)" % (a, b, ep))

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._stage)

        self.assertEqual(self._stage.GetStartTimeCode(), self.START_TIMECODE)
        self.assertEqual(self._stage.GetEndTimeCode(), self.END_TIMECODE)

    def _ValidateUsdLuxLight(self, lightTypeName):
        primPathFormat = '/RfMLightsTest/Lights/%s'

        lightPrimPath = primPathFormat % lightTypeName
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        testNumber = None
        if lightTypeName == 'CylinderLight':
            self.assertTrue(lightPrim.IsA(UsdLux.CylinderLight))
            testNumber = 1
        elif lightTypeName == 'DiskLight':
            self.assertTrue(lightPrim.IsA(UsdLux.DiskLight))
            testNumber = 2
        elif lightTypeName == 'DistantLight':
            self.assertTrue(lightPrim.IsA(UsdLux.DistantLight))
            testNumber = 3
        elif lightTypeName == 'DomeLight':
            self.assertTrue(lightPrim.IsA(UsdLux.DomeLight))
            testNumber = 4
        elif lightTypeName == 'MeshLight':
            self.assertTrue(lightPrim.IsA(UsdLux.GeometryLight))
            testNumber = 5
        elif lightTypeName == 'RectLight':
            self.assertTrue(lightPrim.IsA(UsdLux.RectLight))
            testNumber = 6
        elif lightTypeName == 'SphereLight':
            self.assertTrue(lightPrim.IsA(UsdLux.SphereLight))
            testNumber = 7
        elif lightTypeName == 'AovLight':
            self.assertTrue(lightPrim.GetTypeName(), "PxrAovLight")
            testNumber = 8
        elif lightTypeName == 'EnvDayLight':
            self.assertTrue(lightPrim.GetTypeName(), "PxrEnvDayLight")
            testNumber = 9
        else:
            raise NotImplementedError('Invalid light type %s' % lightTypeName)

        usdVersion = Usd.GetVersion()
        if usdVersion < (0, 21, 11):
            lightSchema = UsdLux.Light(lightPrim)
        else:
            lightSchema = UsdLux.LightAPI(lightPrim)
        self.assertTrue(lightSchema)

        if lightTypeName == 'AovLight':
            # PxrAovLight doesn't have any of the below attributes.
            return

        expectedIntensity = 1.0 + (testNumber * 0.1)
        self._assertGfIsClose(lightSchema.GetIntensityAttr().Get(),
            expectedIntensity, 1e-6)
        self.assertTrue(Gf.IsClose(lightSchema.GetIntensityAttr().Get(),
            expectedIntensity, 1e-6))

        expectedExposure = 0.1 * testNumber
        self._assertGfIsClose(lightSchema.GetExposureAttr().Get(),
            expectedExposure, 1e-6)

        expectedDiffuse = 1.0 + (testNumber * 0.1)
        self._assertGfIsClose(lightSchema.GetDiffuseAttr().Get(),
            expectedDiffuse, 1e-6)

        expectedSpecular = 1.0 + (testNumber * 0.1)
        self._assertGfIsClose(lightSchema.GetSpecularAttr().Get(),
            expectedSpecular, 1e-6)

        if lightTypeName == 'EnvDayLight':
            # PxrEnvDayLight doesn't have any of the below attributes.
            return

        if lightTypeName == 'DomeLight':
            # PxrDomeLight has no normalize attribute
            self.assertFalse(
                lightSchema.GetNormalizeAttr().HasAuthoredValue())
        else:
            expectedNormalize = True
            self.assertEqual(lightSchema.GetNormalizeAttr().Get(),
                expectedNormalize)

        expectedColor = Gf.Vec3f(0.1 * testNumber)
        self._assertGfIsClose(lightSchema.GetColorAttr().Get(),
            expectedColor, 1e-6)

        expectedEnableTemperature = True
        self.assertEqual(lightSchema.GetEnableColorTemperatureAttr().Get(),
            expectedEnableTemperature)

        expectedTemperature = 6500.0 + testNumber
        self._assertGfIsClose(lightSchema.GetColorTemperatureAttr().Get(),
            expectedTemperature, 1e-6)

    def _ValidateDiskLightXformAnimation(self):
        lightPrimPath = '/RfMLightsTest/Lights/DiskLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        diskLight = UsdLux.DiskLight(lightPrim)
        self.assertTrue(diskLight)

        xformOps = diskLight.GetOrderedXformOps()
        self.assertEqual(len(xformOps), 1)

        translateOp = xformOps[0]

        self.assertEqual(translateOp.GetOpName(), 'xformOp:translate')
        self.assertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)

        for frame in range(int(self.START_TIMECODE), int(self.END_TIMECODE + 1.0)):
            expectedTranslation = Gf.Vec3d(2.0, float(frame), 2.0)
            self.assertTrue(
                Gf.IsClose(translateOp.Get(frame), expectedTranslation, 1e-6))

    def _ValidateUsdLuxDistantLightAngle(self):
        lightPrimPath = '/RfMLightsTest/Lights/DistantLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        distantLight = UsdLux.DistantLight(lightPrim)
        self.assertTrue(distantLight)

        expectedAngle = 0.73
        self._assertGfIsClose(distantLight.GetAngleAttr().Get(),
            expectedAngle, 1e-6)

    def _ValidateUsdLuxRectLightTextureFile(self):
        lightPrimPath = '/RfMLightsTest/Lights/RectLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        rectLight = UsdLux.RectLight(lightPrim)
        self.assertTrue(rectLight)

        expectedTextureFile = './RectLight_texture.tex'
        self.assertEqual(rectLight.GetTextureFileAttr().Get(),
            expectedTextureFile)

    def _ValidateUsdLuxDomeLightTextureFile(self):
        lightPrimPath = '/RfMLightsTest/Lights/DomeLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        domeLight = UsdLux.DomeLight(lightPrim)
        self.assertTrue(domeLight)

        expectedTextureFile = './DomeLight_texture.tex'
        self.assertEqual(domeLight.GetTextureFileAttr().Get(),
            expectedTextureFile)

    def _ValidateUsdRiPxrAovLight(self):
        lightPrimPath = '/RfMLightsTest/Lights/AovLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        aovLight = self._stage.DefinePrim(lightPrimPath, "PxrAovLight")
        self.assertTrue(aovLight)

        expectedAovName = 'testAovName'
        self.assertEqual(aovLight.GetAttribute("inputs:ri:light:aovName").Get(), 
                expectedAovName)

        expectedInPrimaryHit = False
        self.assertEqual(aovLight.GetAttribute("inputs:ri:light:inPrimaryHit").Get(),
            expectedInPrimaryHit)

        expectedInReflection = True
        self.assertEqual(aovLight.GetAttribute("inputs:ri:light:inReflection").Get(),
            expectedInReflection)

        expectedInRefraction = True
        self.assertEqual(aovLight.GetAttribute("inputs:ri:light:inRefraction").Get(),
            expectedInRefraction)

        expectedInvert = True
        self.assertEqual(aovLight.GetAttribute("inputs:ri:light:invert").Get(), 
                expectedInvert)

        expectedOnVolumeBoundaries = False
        self.assertEqual(aovLight.GetAttribute("inputs:ri:light:onVolumeBoundaries").Get(),
            expectedOnVolumeBoundaries)

        expectedUseColor = True
        self.assertEqual(aovLight.GetAttribute("inputs:ri:light:useColor").Get(), 
                expectedUseColor)

        expectedUseThroughput = False
        self.assertEqual(aovLight.GetAttribute("inputs:ri:light:useThroughput").Get(),
            expectedUseThroughput)

    def _ValidateUsdRiPxrEnvDayLight(self):
        lightPrimPath = '/RfMLightsTest/Lights/EnvDayLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        envDayLight = self._stage.DefinePrim(lightPrimPath, "PxrEnvDayLight")
        self.assertTrue(envDayLight)

        expectedDay = 9
        self.assertEqual(envDayLight.GetAttribute("inputs:ri:light:day").Get(), 
                expectedDay)

        expectedHaziness = 1.9
        self._assertGfIsClose(envDayLight.GetAttribute("inputs:ri:light:haziness").Get(),
            expectedHaziness, 1e-6)

        expectedHour = 9.9
        self._assertGfIsClose(envDayLight.GetAttribute("inputs:ri:light:hour").Get(),
            expectedHour, 1e-6)

        expectedLatitude = 90.0
        self._assertGfIsClose(envDayLight.GetAttribute("inputs:ri:light:latitude").Get(),
            expectedLatitude, 1e-6)

        expectedLongitude = -90.0
        self._assertGfIsClose(envDayLight.GetAttribute("inputs:ri:light:longitude").Get(),
            expectedLongitude, 1e-6)

        expectedMonth = 9
        self.assertEqual(envDayLight.GetAttribute("inputs:ri:light:month").Get(), expectedMonth)

        expectedSkyTint = Gf.Vec3f(0.9)
        self._assertGfIsClose(envDayLight.GetAttribute("inputs:ri:light:skyTint").Get(),
            expectedSkyTint, 1e-6)

        expectedSunDirection = Gf.Vec3f(0.0, 0.0, 0.9)
        self._assertGfIsClose(envDayLight.GetAttribute("inputs:ri:light:sunDirection").Get(),
            expectedSunDirection, 1e-6)

        expectedSunSize = 0.9
        self._assertGfIsClose(envDayLight.GetAttribute("inputs:ri:light:sunSize").Get(),
            expectedSunSize, 1e-6)

        expectedSunTint = Gf.Vec3f(0.9)
        self._assertGfIsClose(envDayLight.GetAttribute("inputs:ri:light:sunTint").Get(),
            expectedSunTint, 1e-6)

        expectedYear = 2019
        self.assertEqual(envDayLight.GetAttribute("inputs:ri:light:year").Get(), expectedYear)

        expectedZone = 9.0
        self._assertGfIsClose(envDayLight.GetAttribute("inputs:ri:light:zone").Get(),
            expectedZone, 1e-6)

    def _ValidateUsdLuxShapingAPI(self):
        lightPrimPath = '/RfMLightsTest/Lights/DiskLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        self.assertTrue(lightPrim.HasAPI(UsdLux.ShapingAPI))

        shapingAPI = UsdLux.ShapingAPI(lightPrim)
        self.assertTrue(shapingAPI)

        expectedFocus = 0.2
        self._assertGfIsClose(shapingAPI.GetShapingFocusAttr().Get(),
            expectedFocus, 1e-6)

        expectedFocusTint = Gf.Vec3f(0.2)
        self._assertGfIsClose(shapingAPI.GetShapingFocusTintAttr().Get(),
            expectedFocusTint, 1e-6)

        expectedConeAngle = 92.0
        self._assertGfIsClose(shapingAPI.GetShapingConeAngleAttr().Get(),
            expectedConeAngle, 1e-6)

        expectedConeSoftness = 0.2
        self._assertGfIsClose(shapingAPI.GetShapingConeSoftnessAttr().Get(),
            expectedConeSoftness, 1e-6)

        expectedProfilePath = './DiskLight_profile.ies'
        self.assertEqual(shapingAPI.GetShapingIesFileAttr().Get(),
            expectedProfilePath)

        expectedProfileScale = 1.2
        self._assertGfIsClose(shapingAPI.GetShapingIesAngleScaleAttr().Get(),
            expectedProfileScale, 1e-6)

    def _ValidateUsdLuxSuppressed(self):
        ''' makes sure suppressed properties are not written out '''
        lightPrimPath = '/RfMLightsTest/Lights/DiskLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertFalse(bool(lightPrim.GetAttribute('inputs:ri:light:shadowSubset')))

    def _ValidateUsdLuxShadowAPI(self):
        lightPrimPath = '/RfMLightsTest/Lights/RectLight'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)

        self.assertTrue(lightPrim.HasAPI(UsdLux.ShadowAPI))

        shadowAPI = UsdLux.ShadowAPI(lightPrim)
        self.assertTrue(shadowAPI)

        self.assertTrue(shadowAPI.GetShadowEnableAttr().Get())

        expectedShadowColor = Gf.Vec3f(0.6)
        self._assertGfIsClose(shadowAPI.GetShadowColorAttr().Get(),
            expectedShadowColor, 1e-6)

        expectedShadowDistance = -0.6
        self._assertGfIsClose(shadowAPI.GetShadowDistanceAttr().Get(),
            expectedShadowDistance, 1e-6)

        expectedShadowFalloff = -0.6
        self._assertGfIsClose(shadowAPI.GetShadowFalloffAttr().Get(),
            expectedShadowFalloff, 1e-6)

        expectedShadowFalloffGamma = 0.6
        self._assertGfIsClose(shadowAPI.GetShadowFalloffGammaAttr().Get(),
            expectedShadowFalloffGamma, 1e-6)

    def testExportCylinderLight(self):
        self._ValidateUsdLuxLight('CylinderLight')

    def testExportDiskLight(self):
        self._ValidateUsdLuxLight('DiskLight')
        self._ValidateDiskLightXformAnimation()
        self._ValidateUsdLuxShapingAPI()
        self._ValidateUsdLuxSuppressed()

    def testExportDistantLight(self):
        self._ValidateUsdLuxLight('DistantLight')
        self._ValidateUsdLuxDistantLightAngle()

    def testExportDomeLight(self):
        self._ValidateUsdLuxLight('DomeLight')
        self._ValidateUsdLuxDomeLightTextureFile()

    def testExportMeshLight(self):
        self._ValidateUsdLuxLight('MeshLight')

    def testExportRectLight(self):
        self._ValidateUsdLuxLight('RectLight')
        self._ValidateUsdLuxRectLightTextureFile()
        self._ValidateUsdLuxShadowAPI()

    def testExportSphereLight(self):
        self._ValidateUsdLuxLight('SphereLight')

    def testExportAovLight(self):
        self._ValidateUsdLuxLight('AovLight')
        self._ValidateUsdRiPxrAovLight()

    def testExportEnvDayLight(self):
        self._ValidateUsdLuxLight('EnvDayLight')
        self._ValidateUsdRiPxrEnvDayLight()


if __name__ == '__main__':
    unittest.main(verbosity=2)
