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
from pxr import Usd

from maya import OpenMaya
from maya import OpenMayaAnim
from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdImportRfMLight(unittest.TestCase):

    START_TIMECODE = 1.0
    END_TIMECODE = 5.0

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.readOnlySetUpClass(__file__)
        cmds.file(new=True, force=True)

        cmds.loadPlugin('RenderMan_for_Maya', quiet=True)

        # Import from USD.
        usdFilePath = os.path.join(inputPath, "UsdImportRfMLightTest", "RfMLightsTest.usda")
        cmds.usdImport(file=usdFilePath, shadingMode=[['pxrRis', 'default'], ], readAnimData=True)

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
        Tests that the USD stage being imported opens successfully.
        """
        self.assertTrue(self._stage)

        self.assertEqual(self._stage.GetStartTimeCode(), self.START_TIMECODE)
        self.assertEqual(self._stage.GetEndTimeCode(), self.END_TIMECODE)

    def _GetMayaDependencyNode(self, objectName):
         selectionList = OpenMaya.MSelectionList()
         selectionList.add(objectName)
         mObj = OpenMaya.MObject()
         selectionList.getDependNode(0, mObj)

         depNodeFn = OpenMaya.MFnDependencyNode(mObj)
         self.assertTrue(depNodeFn)

         return depNodeFn

    def _ValidateMayaLight(self, lightTypeName):
        nodePathFormat = '|RfMLightsTest|Lights|{lightTypeName}|{lightTypeName}Shape'
        nodePath = nodePathFormat.format(lightTypeName=lightTypeName)

        depNodeFn = self._GetMayaDependencyNode(nodePath)

        self.assertTrue(lightTypeName in cmds.listConnections('defaultLightSet'))

        testNumber = None
        if lightTypeName == 'CylinderLight':
            self.assertEqual(depNodeFn.typeName(), 'PxrCylinderLight')
            testNumber = 1
        elif lightTypeName == 'DiskLight':
            self.assertEqual(depNodeFn.typeName(), 'PxrDiskLight')
            testNumber = 2
        elif lightTypeName == 'DistantLight':
            self.assertEqual(depNodeFn.typeName(), 'PxrDistantLight')
            testNumber = 3
        elif lightTypeName == 'DomeLight':
            self.assertEqual(depNodeFn.typeName(), 'PxrDomeLight')
            testNumber = 4
        elif lightTypeName == 'MeshLight':
            self.assertEqual(depNodeFn.typeName(), 'PxrMeshLight')
            testNumber = 5
        elif lightTypeName == 'RectLight':
            self.assertEqual(depNodeFn.typeName(), 'PxrRectLight')
            testNumber = 6
        elif lightTypeName == 'SphereLight':
            self.assertEqual(depNodeFn.typeName(), 'PxrSphereLight')
            testNumber = 7
        elif lightTypeName == 'AovLight':
            self.assertEqual(depNodeFn.typeName(), 'PxrAovLight')
            testNumber = 8
        elif lightTypeName == 'EnvDayLight':
            self.assertEqual(depNodeFn.typeName(), 'PxrEnvDayLight')
            testNumber = 9
        else:
            raise NotImplementedError('Invalid light type %s' % lightTypeName)

        if lightTypeName == 'AovLight':
            # PxrAovLight doesn't have any of the below attributes.
            return

        expectedIntensity = 1.0 + (testNumber * 0.1)
        self._assertGfIsClose(cmds.getAttr('%s.intensity' % nodePath),
            expectedIntensity, 1e-6)

        expectedExposure = 0.1 * testNumber
        self._assertGfIsClose(cmds.getAttr('%s.exposure' % nodePath),
            expectedExposure, 1e-6)

        expectedDiffuse = 1.0 + (testNumber * 0.1)
        self._assertGfIsClose(cmds.getAttr('%s.diffuse' % nodePath),
            expectedDiffuse, 1e-6)

        expectedSpecular = 1.0 + (testNumber * 0.1)
        self._assertGfIsClose(cmds.getAttr('%s.specular' % nodePath),
            expectedSpecular, 1e-6)

        if lightTypeName == 'EnvDayLight':
            # PxrEnvDayLight doesn't have any of the below attributes.
            return

        # PxrDomeLight has no normalize attribute
        if lightTypeName != 'DomeLight':
            self.assertTrue(cmds.getAttr('%s.areaNormalize' % nodePath))

        expectedColor = 0.1 * testNumber
        self._assertGfIsClose(cmds.getAttr('%s.lightColorR' % nodePath),
            expectedColor, 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.lightColorG' % nodePath),
            expectedColor, 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.lightColorB' % nodePath),
            expectedColor, 1e-6)

        self.assertTrue(cmds.getAttr('%s.enableTemperature' % nodePath))

        expectedTemperature = 6500.0 + testNumber
        self._assertGfIsClose(cmds.getAttr('%s.temperature' % nodePath),
            expectedTemperature, 1e-6)

    def _ValidatePxrDiskLightTransformAnimation(self):
        nodePath = '|RfMLightsTest|Lights|DiskLight'

        depNodeFn = self._GetMayaDependencyNode(nodePath)

        animatedPlugs = OpenMaya.MPlugArray()
        OpenMayaAnim.MAnimUtil.findAnimatedPlugs(depNodeFn.object(),
            animatedPlugs)
        self.assertEqual(animatedPlugs.length(), 1)

        translateYPlug = animatedPlugs[0]
        self.assertEqual(translateYPlug.name(), 'DiskLight.translateY')

        animObjs = OpenMaya.MObjectArray()
        OpenMayaAnim.MAnimUtil.findAnimation(translateYPlug, animObjs)
        self.assertEqual(animObjs.length(), 1)

        animCurveFn = OpenMayaAnim.MFnAnimCurve(animObjs[0])
        timeUnit = OpenMaya.MTime.uiUnit()
        for frame in range(int(self.START_TIMECODE), int(self.END_TIMECODE + 1.0)):
            value = animCurveFn.evaluate(OpenMaya.MTime(frame, timeUnit))
            self._assertGfIsClose(float(frame), value, 1e-6)

    def _ValidatePxrDistantLightAngle(self):
        nodePath = '|RfMLightsTest|Lights|DistantLight|DistantLightShape'

        expectedAngle = 0.73
        self._assertGfIsClose(cmds.getAttr('%s.angleExtent' % nodePath),
            expectedAngle, 1e-6)

    def _ValidatePxrRectLightTextureFile(self):
        nodePath = '|RfMLightsTest|Lights|RectLight|RectLightShape'

        expectedTextureFile = './RectLight_texture.tex'
        self.assertEqual(cmds.getAttr('%s.lightColorMap' % nodePath),
            expectedTextureFile)

    def _ValidatePxrDomeLightTextureFile(self):
        nodePath = '|RfMLightsTest|Lights|DomeLight|DomeLightShape'

        expectedTextureFile = './DomeLight_texture.tex'
        self.assertEqual(cmds.getAttr('%s.lightColorMap' % nodePath),
            expectedTextureFile)

    def _ValidatePxrAovLight(self):
        nodePath = '|RfMLightsTest|Lights|AovLight|AovLightShape'

        expectedAovName = 'testAovName'
        self.assertEqual(cmds.getAttr('%s.aovName' % nodePath),
            expectedAovName)

        expectedInPrimaryHit = False
        self.assertEqual(cmds.getAttr('%s.inPrimaryHit' % nodePath),
            expectedInPrimaryHit)

        expectedInReflection = True
        self.assertEqual(cmds.getAttr('%s.inReflection' % nodePath),
            expectedInReflection)

        expectedInRefraction = True
        self.assertEqual(cmds.getAttr('%s.inRefraction' % nodePath),
            expectedInRefraction)

        expectedInvert = True
        self.assertEqual(cmds.getAttr('%s.invert' % nodePath), expectedInvert)

        expectedOnVolumeBoundaries = False
        self.assertEqual(cmds.getAttr('%s.onVolumeBoundaries' % nodePath),
            expectedOnVolumeBoundaries)

        expectedUseColor = True
        self.assertEqual(cmds.getAttr('%s.useColor' % nodePath),
            expectedUseColor)

        expectedUseThroughput = False
        self.assertEqual(cmds.getAttr('%s.useThroughput' % nodePath),
            expectedUseThroughput)

    def _ValidatePxrEnvDayLight(self):
        nodePath = '|RfMLightsTest|Lights|EnvDayLight|EnvDayLightShape'

        expectedDay = 9
        self.assertEqual(cmds.getAttr('%s.day' % nodePath), expectedDay)

        expectedHaziness = 1.9
        self._assertGfIsClose(cmds.getAttr('%s.haziness' % nodePath),
            expectedHaziness, 1e-6)

        expectedHour = 9.9
        self._assertGfIsClose(cmds.getAttr('%s.hour' % nodePath),
            expectedHour, 1e-6)

        expectedLatitude = 90.0
        self._assertGfIsClose(cmds.getAttr('%s.latitude' % nodePath),
            expectedLatitude, 1e-6)

        expectedLongitude = -90.0
        self._assertGfIsClose(cmds.getAttr('%s.longitude' % nodePath),
            expectedLongitude, 1e-6)

        expectedMonth = 9
        self.assertEqual(cmds.getAttr('%s.month' % nodePath), expectedMonth)

        expectedSkyTint = Gf.Vec3f(0.9)
        self._assertGfIsClose(cmds.getAttr('%s.skyTintR' % nodePath),
            expectedSkyTint[0], 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.skyTintG' % nodePath),
            expectedSkyTint[1], 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.skyTintB' % nodePath),
            expectedSkyTint[2], 1e-6)

        expectedSunDirection = Gf.Vec3f(0.0, 0.0, 0.9)
        self._assertGfIsClose(cmds.getAttr('%s.sunDirectionX' % nodePath),
            expectedSunDirection[0], 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.sunDirectionY' % nodePath),
            expectedSunDirection[1], 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.sunDirectionZ' % nodePath),
            expectedSunDirection[2], 1e-6)

        expectedSunSize = 0.9
        self._assertGfIsClose(cmds.getAttr('%s.sunSize' % nodePath),
            expectedSunSize, 1e-6)

        expectedSunTint = Gf.Vec3f(0.9)
        self._assertGfIsClose(cmds.getAttr('%s.sunTintR' % nodePath),
            expectedSunTint[0], 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.sunTintG' % nodePath),
            expectedSunTint[1], 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.sunTintB' % nodePath),
            expectedSunTint[2], 1e-6)

        expectedYear = 2019
        self.assertEqual(cmds.getAttr('%s.year' % nodePath), expectedYear)

        expectedZone = 9.0
        self._assertGfIsClose(cmds.getAttr('%s.zone' % nodePath),
            expectedZone, 1e-6)

    def _ValidateMayaLightShaping(self):
        nodePath = '|RfMLightsTest|Lights|DiskLight|DiskLightShape'

        expectedFocus = 0.2
        self._assertGfIsClose(cmds.getAttr('%s.emissionFocus' % nodePath),
            expectedFocus, 1e-6)

        expectedFocusTint = 0.2
        self._assertGfIsClose(cmds.getAttr('%s.emissionFocusTintR' % nodePath),
            expectedFocusTint, 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.emissionFocusTintG' % nodePath),
            expectedFocusTint, 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.emissionFocusTintB' % nodePath),
            expectedFocusTint, 1e-6)

        expectedConeAngle = 92.0
        self._assertGfIsClose(cmds.getAttr('%s.coneAngle' % nodePath),
            expectedConeAngle, 1e-6)

        expectedConeSoftness = 0.2
        self._assertGfIsClose(cmds.getAttr('%s.coneSoftness' % nodePath),
            expectedConeSoftness, 1e-6)

        expectedProfilePath = './DiskLight_profile.ies'
        self.assertEqual(cmds.getAttr('%s.iesProfile' % nodePath),
            expectedProfilePath)

        expectedProfileScale = 1.2
        self._assertGfIsClose(cmds.getAttr('%s.iesProfileScale' % nodePath),
            expectedProfileScale, 1e-6)

        expectedProfileNormalize = False
        self.assertEqual(cmds.getAttr('%s.iesProfileNormalize' % nodePath),
            expectedProfileNormalize)

        expectedProfileNormalize = False
        self.assertEqual(cmds.getAttr('%s.iesProfileNormalize' % nodePath),
            expectedProfileNormalize)

    def _ValidateMayaLightShadow(self):
        nodePath = '|RfMLightsTest|Lights|RectLight|RectLightShape'

        expectedShadowsEnabled = True
        self.assertEqual(cmds.getAttr('%s.enableShadows' % nodePath),
            expectedShadowsEnabled)

        expectedShadowColor = 0.6
        self._assertGfIsClose(cmds.getAttr('%s.shadowColorR' % nodePath),
            expectedShadowColor, 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.shadowColorG' % nodePath),
            expectedShadowColor, 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.shadowColorB' % nodePath),
            expectedShadowColor, 1e-6)

        expectedShadowDistance = -0.6
        self._assertGfIsClose(cmds.getAttr('%s.shadowDistance' % nodePath),
            expectedShadowDistance, 1e-6)

        expectedShadowFalloff = -0.6
        self._assertGfIsClose(cmds.getAttr('%s.shadowFalloff' % nodePath),
            expectedShadowFalloff, 1e-6)

        expectedShadowFalloffGamma = 0.6
        self._assertGfIsClose(cmds.getAttr('%s.shadowFalloffGamma' % nodePath),
            expectedShadowFalloffGamma, 1e-6)

    def _ValidatePxrLightImportedUnderScope(self):
        # This tests a case where the "Scope" translator was treating lights as
        # materials and deciding to not translate the scope.  This would result in
        # the "NonRootRectLight" here to be translated at the scene root.
        nodePath = '|RfMLightsTest|NonMaterialsScope|NonRootRectLight'
        depNodeFn = self._GetMayaDependencyNode(nodePath)

        # _GetMayaDependencyNode already does this assert, but in case we ever
        # remove that, just redundantly assert here.
        self.assertTrue(depNodeFn)

    def testImportRenderManForMayaLights(self):
        """
        Tests that UsdLux schema USD prims import into Maya as the appropriate
        RenderMan for Maya light.
        """
        self.assertTrue(cmds.pluginInfo('RenderMan_for_Maya', query=True,
            loaded=True))
        self._ValidatePxrLightImportedUnderScope()

    def testImportCylinderLight(self):
        self._ValidateMayaLight('CylinderLight')

    def testImportDiskLight(self):
        self._ValidateMayaLight('DiskLight')
        self._ValidatePxrDiskLightTransformAnimation()
        self._ValidateMayaLightShaping()

    def testImportDistantLight(self):
        self._ValidateMayaLight('DistantLight')
        self._ValidatePxrDistantLightAngle()

    def testImportDomeLight(self):
        self._ValidateMayaLight('DomeLight')
        self._ValidatePxrDomeLightTextureFile()

    def testImportMeshLight(self):
        self._ValidateMayaLight('MeshLight')

    def testImportRectLight(self):
        self._ValidateMayaLight('RectLight')
        self._ValidatePxrRectLightTextureFile()
        self._ValidateMayaLightShadow()

    def testImportSphereLight(self):
        self._ValidateMayaLight('SphereLight')

    def testImportAovLight(self):
        self._ValidateMayaLight('AovLight')
        self._ValidatePxrAovLight()
    
    def testImportEnvDayLight(self):
        self._ValidateMayaLight('EnvDayLight')
        self._ValidatePxrEnvDayLight()


if __name__ == '__main__':
    unittest.main(verbosity=2)
