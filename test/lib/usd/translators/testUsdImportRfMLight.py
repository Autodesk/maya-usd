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
        if lightTypeName == 'DistantLight':
            self.assertEqual(depNodeFn.typeName(), 'PxrDistantLight')
            testNumber = 3
        elif lightTypeName == 'RectLight':
            self.assertEqual(depNodeFn.typeName(), 'PxrRectLight')
            testNumber = 6
        elif lightTypeName == 'SphereLight':
            self.assertEqual(depNodeFn.typeName(), 'PxrSphereLight')
            testNumber = 7
        else:
            raise NotImplementedError('Invalid light type %s' % lightTypeName)

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

        expectedColor = Gf.ConvertLinearToDisplay(Gf.Vec3f(0.1 * testNumber))
        self._assertGfIsClose(cmds.getAttr('%s.lightColorR' % nodePath),
            expectedColor[0], 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.lightColorG' % nodePath),
            expectedColor[1], 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.lightColorB' % nodePath),
            expectedColor[2], 1e-6)

        self.assertTrue(cmds.getAttr('%s.enableTemperature' % nodePath))

        expectedTemperature = 6500.0 + testNumber
        self._assertGfIsClose(cmds.getAttr('%s.temperature' % nodePath),
            expectedTemperature, 1e-6)


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

    def _ValidateMayaLightShadow(self):
        nodePath = '|RfMLightsTest|Lights|RectLight|RectLightShape'

        expectedShadowsEnabled = True
        self.assertEqual(cmds.getAttr('%s.enableShadows' % nodePath),
            expectedShadowsEnabled)

        expectedShadowColor = Gf.ConvertLinearToDisplay(Gf.Vec3f(0.6))
        self._assertGfIsClose(cmds.getAttr('%s.shadowColorR' % nodePath),
            expectedShadowColor[0], 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.shadowColorG' % nodePath),
            expectedShadowColor[1], 1e-6)
        self._assertGfIsClose(cmds.getAttr('%s.shadowColorB' % nodePath),
            expectedShadowColor[2], 1e-6)

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

    def testImportDistantLight(self):
        self._ValidateMayaLight('DistantLight')
        self._ValidatePxrDistantLightAngle()

    def testImportRectLight(self):
        self._ValidateMayaLight('RectLight')
        self._ValidatePxrRectLightTextureFile()
        self._ValidateMayaLightShadow()

    def testImportSphereLight(self):
        self._ValidateMayaLight('SphereLight')


if __name__ == '__main__':
    unittest.main(verbosity=2)
