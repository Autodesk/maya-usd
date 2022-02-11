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

from maya import OpenMaya
from maya import OpenMayaAnim
from maya import cmds
from maya import standalone

import fixturesUtils

def getMayaAPIVersion():
    version = cmds.about(api=True)
    return int(str(version)[:4])

class testUsdImportLight(unittest.TestCase):

    START_TIMECODE = 1.0
    END_TIMECODE = 5.0

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.readOnlySetUpClass(__file__)
        cmds.file(new=True, force=True)

        # We need to use different input usd files depending on the version of USD.
        # We do this because USD introduced breaking changes on lights in USD 21.*
        # More precisely :
        # 
        # - Before USD 21.* : lights attributes didn't have any prefix. Attributes
        #                     were named "intensity", "color", etc...
        # - In 21.02        : The attributes from light schemas were added a
        #                     namespace "inputs". But attributes from UsdLuxShapingAPI
        #                     (used for spot lights) or UsdLuxShadowsAPI (for shadows)
        #                     were still left without this namespace
        # - In 21.05        : Attributes from UsdLuxShadows and UsdLuxShapingAPI also 
        #                     have a namespace "inputs" 
        testFile = "LightsTest.usda"
        usdVersion = Usd.GetVersion()
        if usdVersion < (0,21,2):
            testFile = "LightsTest_2011.usda"
        elif usdVersion == (0,21,2):
            testFile = "LightsTest_2102.usda"
            
        # Import from USD.
        usdFilePath = os.path.join(inputPath, "UsdImportLightTest", testFile)
        cmds.usdImport(file=usdFilePath, readAnimData=True)
        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

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
   
    def _ValidateMayaDistantLight(self):
        nodePath = 'directionalLight1'        
        depNodeFn = self._GetMayaDependencyNode(nodePath)
        self.assertTrue(depNodeFn)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.intensity' % nodePath),
            2, 1e-6))
        
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorR' % nodePath),
            1, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorG' % nodePath),
            0.9, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorB' % nodePath),
            0.8, 1e-6))
        
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.rotateX' % nodePath),
            -20, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.rotateY' % nodePath),
            -40, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.rotateZ' % nodePath),
            0, 1e-6))

        shadowColorList = cmds.getAttr('%s.shadowColor' % nodePath)
        self.assertTrue(Gf.IsClose(shadowColorList[0],
            Gf.Vec3f(0.1, 0.2, 0.3), 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.lightAngle' % nodePath),
            1.5, 1e-6))       
        # verify the animation is imported properly, check at frame 5
        cmds.currentTime(5)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.lightAngle' % nodePath),
            2, 1e-6))
        cmds.currentTime(1)

    def _ValidateMayaPointLight(self):
        nodePath = 'pointLight1'
        depNodeFn = self._GetMayaDependencyNode(nodePath)
        cmds.currentTime(1)
        self.assertTrue(cmds.getAttr('%s.emitDiffuse' % nodePath) == 1)
        self.assertTrue(cmds.getAttr('%s.emitSpecular' % nodePath) == 0)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.translateX' % nodePath),
            -10, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.translateY' % nodePath),
            10, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.translateZ' % nodePath),
            0, 1e-6))

        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorR' % nodePath),
            1, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorG' % nodePath),
            0.5, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorB' % nodePath),
            0.1, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.intensity' % nodePath),
            0.5, 1e-6))
        # verify the animation is imported properly, check at frame 5
        cmds.currentTime(5)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.intensity' % nodePath),
            2, 1e-6))
        cmds.currentTime(1)
        
    def _ValidateMayaSpotLight(self):
        nodePath = 'spotLight1'
        depNodeFn = self._GetMayaDependencyNode(nodePath)
        
        self.assertTrue(cmds.getAttr('%s.emitDiffuse' % nodePath) == 0)
        self.assertTrue(cmds.getAttr('%s.emitSpecular' % nodePath) == 1)

        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.translateX' % nodePath),
            10, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.translateY' % nodePath),
            7, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.translateZ' % nodePath),
            -8, 1e-6))

        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.rotateX' % nodePath),
            -45, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.rotateY' % nodePath),
            90, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.rotateZ' % nodePath),
            -5, 1e-6))

        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorR' % nodePath),
            0.3, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorG' % nodePath),
            1, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorB' % nodePath),
            0.2, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.intensity' % nodePath),
            0.8, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.dropoff' % nodePath),
            8, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.coneAngle' % nodePath),
            30, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.penumbraAngle' % nodePath),
            10, 1e-6))
        # verify the animation is imported properly, check at frame 5
        cmds.currentTime(5)
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorR' % nodePath),
            0, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorG' % nodePath),
            0.2, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorB' % nodePath),
            0.1, 1e-6))

        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.translateX' % nodePath),
            5, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.translateY' % nodePath),
            3, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.translateZ' % nodePath),
            8, 1e-6))
        cmds.currentTime(1)

    def _ValidateMayaAreaLight(self):
        nodePath = 'areaLight1'
        depNodeFn = self._GetMayaDependencyNode(nodePath)
        
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.translateX' % nodePath),
            8, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.translateY' % nodePath),
            0, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.translateZ' % nodePath),
            10, 1e-6))

        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.rotateX' % nodePath),
            0, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.rotateY' % nodePath),
            23, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.rotateZ' % nodePath),
            0, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.scaleX' % nodePath),
            4, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.scaleY' % nodePath),
            3, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.scaleZ' % nodePath),
            2, 1e-6))


        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorR' % nodePath),
            0.8, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorG' % nodePath),
            0.7, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorB' % nodePath),
            0.6, 1e-6))
        if getMayaAPIVersion() > 2019:
            self.assertTrue(cmds.getAttr('%s.normalize' % nodePath) == 0)
   
    def _ValidateLightImportedUnderScope(self):
        # This tests a case where the "Scope" translator was treating lights as
        # materials and deciding to not translate the scope.  This would result in
        # the "NonRootRectLight" here to be translated at the scene root.
        nodePath = '|LightsTest|NonMaterialsScope|NonRootRectLight'
        depNodeFn = self._GetMayaDependencyNode(nodePath)

        # _GetMayaDependencyNode already does this assert, but in case we ever
        # remove that, just redundantly assert here.
        self.assertTrue(depNodeFn)

    def testImportMayaLights(self):
        """
        Tests that UsdLux schema USD prims import into Maya as the appropriate Maya light.
        """                
        self._ValidateMayaDistantLight()
        self._ValidateMayaPointLight()
        self._ValidateMayaSpotLight()
        self._ValidateMayaAreaLight()
        self._ValidateLightImportedUnderScope()



if __name__ == '__main__':
    unittest.main(verbosity=2)
