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

class testUsdExportLight(unittest.TestCase):

    START_TIMECODE = 1.0
    END_TIMECODE = 5.0

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        mayaFile = os.path.join(inputPath, 'UsdExportLightTest', 'UsdExportLightTest.ma')
        cmds.file(mayaFile, open=True, force=True)

        # Export to USD.
        cls._usdFilePath = os.path.abspath('UsdExportLightTest.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=cls._usdFilePath,
            frameRange=(cls.START_TIMECODE, cls.END_TIMECODE))

        cls._stage = Usd.Stage.Open(cls._usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._stage)

        self.assertEqual(self._stage.GetStartTimeCode(), self.START_TIMECODE)
        self.assertEqual(self._stage.GetEndTimeCode(), self.END_TIMECODE)

    def _ValidateDistantLight(self):
        lightPrimPath = '/directionalLight1'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)
        distantLight = UsdLux.DistantLight(lightPrim)
        self.assertTrue(distantLight)

        self.assertTrue(Gf.IsClose(distantLight.GetIntensityAttr().Get(1),
            2, 1e-6))
        self.assertTrue(Gf.IsClose(distantLight.GetColorAttr().Get(1), Gf.Vec3f(1, 0.9, 0.8), 1e-6))

        self.assertTrue(Gf.IsClose(distantLight.GetAngleAttr().Get(1),
            1.5, 1e-6))
        self.assertTrue(Gf.IsClose(distantLight.GetAngleAttr().Get(5),
            2, 1e-6))

        rotateOp = distantLight.GetOrderedXformOps()
        self.assertEqual(rotateOp[0].GetOpType(), UsdGeom.XformOp.TypeRotateXYZ)
        self.assertTrue(UsdGeom.XformCommonAPI(distantLight))
        self.assertTrue(Gf.IsClose(rotateOp[0].Get(1), Gf.Vec3f(-20, -40, 0.0), 1e-6))
       

        self.assertTrue(lightPrim.HasAPI(UsdLux.ShadowAPI))
        shadowAPI = UsdLux.ShadowAPI(lightPrim)
        self.assertTrue(shadowAPI)

        self.assertTrue(shadowAPI.GetShadowEnableAttr().Get(1))
        self.assertTrue(Gf.IsClose(shadowAPI.GetShadowColorAttr().Get(1), Gf.Vec3f(0.1, 0.2, 0.3), 1e-6))
        return

    def _ValidatePointLight(self):
        lightPrimPath = '/pointLight1'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)
        sphereLight = UsdLux.SphereLight(lightPrim)
        self.assertTrue(sphereLight)
        self.assertTrue(Gf.IsClose(sphereLight.GetColorAttr().Get(1), Gf.Vec3f(1, 0.5, 0.1), 1e-6))

        self.assertTrue(Gf.IsClose(sphereLight.GetIntensityAttr().Get(1), 0.5, 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetIntensityAttr().Get(5), 2, 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetSpecularAttr().Get(1), 0, 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetTreatAsPointAttr().Get(1), 1, 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetRadiusAttr().Get(1), 0, 1e-6))
    
        translateOp = sphereLight.GetOrderedXformOps()
        self.assertEqual(translateOp[0].GetOpType(), UsdGeom.XformOp.TypeTranslate)
        self.assertTrue(UsdGeom.XformCommonAPI(sphereLight))
        self.assertTrue(Gf.IsClose(translateOp[0].Get(1), Gf.Vec3d(-10, 10, 0.0), 1e-6))
    

    def _ValidateSpotLight(self):
        lightPrimPath = '/spotLight1'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)
        sphereLight = UsdLux.SphereLight(lightPrim)
        self.assertTrue(sphereLight)

        self.assertTrue(Gf.IsClose(sphereLight.GetColorAttr().Get(1), Gf.Vec3f(0.3, 1, 0.2), 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetColorAttr().Get(5), Gf.Vec3f(0, 0.2, 0.1), 1e-6))

        self.assertTrue(Gf.IsClose(sphereLight.GetIntensityAttr().Get(1), 0.8, 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetDiffuseAttr().Get(1), 0, 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetTreatAsPointAttr().Get(1), 1, 1e-6))
        self.assertTrue(Gf.IsClose(sphereLight.GetRadiusAttr().Get(1), 0, 1e-6))
    
        (translateOp,rotateOp) = sphereLight.GetOrderedXformOps()
        self.assertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)
        self.assertEqual(rotateOp.GetOpType(), UsdGeom.XformOp.TypeRotateXYZ)
        self.assertTrue(UsdGeom.XformCommonAPI(sphereLight))
        self.assertTrue(Gf.IsClose(translateOp.Get(1), Gf.Vec3d(10, 7, -8), 1e-6))
        self.assertTrue(Gf.IsClose(translateOp.Get(5), Gf.Vec3d(5, 3, 8), 1e-6))
        self.assertTrue(Gf.IsClose(rotateOp.Get(1), Gf.Vec3f(-45, 90, -5), 1e-6))
        
        self.assertTrue(lightPrim.HasAPI(UsdLux.ShadowAPI))
        shadowAPI = UsdLux.ShadowAPI(lightPrim)
        self.assertTrue(shadowAPI)

        self.assertTrue(lightPrim.HasAPI(UsdLux.ShapingAPI))
        shapingAPI = UsdLux.ShapingAPI(lightPrim)
        self.assertTrue(shapingAPI)
        self.assertTrue(Gf.IsClose(shapingAPI.GetShapingConeAngleAttr().Get(1), 25, 1e-6))
        self.assertTrue(Gf.IsClose(shapingAPI.GetShapingConeSoftnessAttr().Get(1), 0.4, 1e-6))
        self.assertTrue(Gf.IsClose(shapingAPI.GetShapingFocusAttr().Get(1), 8, 1e-6))

    def _ValidateAreaLight(self):
        lightPrimPath = '/areaLight1'
        lightPrim = self._stage.GetPrimAtPath(lightPrimPath)
        self.assertTrue(lightPrim)
        rectLight = UsdLux.RectLight(lightPrim)
        self.assertTrue(rectLight)
        self.assertTrue(Gf.IsClose(rectLight.GetColorAttr().Get(), Gf.Vec3f(0.8, 0.7, 0.6), 1e-6))
        self.assertTrue(Gf.IsClose(rectLight.GetIntensityAttr().Get(), 1.2, 1e-6))
        self.assertTrue(rectLight.GetNormalizeAttr().Get())
        
        self.assertTrue(lightPrim.HasAPI(UsdLux.ShadowAPI))
        shadowAPI = UsdLux.ShadowAPI(lightPrim)
        self.assertTrue(shadowAPI)

        (translateOp,rotateOp,scaleOp) = rectLight.GetOrderedXformOps()
        self.assertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)
        self.assertEqual(rotateOp.GetOpType(), UsdGeom.XformOp.TypeRotateXYZ)
        self.assertEqual(scaleOp.GetOpType(), UsdGeom.XformOp.TypeScale)
        self.assertTrue(UsdGeom.XformCommonAPI(rectLight))
        self.assertTrue(Gf.IsClose(translateOp.Get(1), Gf.Vec3d(8, 0, 10), 1e-6))
        self.assertTrue(Gf.IsClose(scaleOp.Get(1), Gf.Vec3f(4,3,2), 1e-6))
        self.assertTrue(Gf.IsClose(rotateOp.Get(1), Gf.Vec3f(0,23,0), 1e-6))
        
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
        cmds.currentTime(1)
        self.assertTrue(depNodeFn)
        
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorR' % nodePath),
            1, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorG' % nodePath),
            0.9, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.colorB' % nodePath),
            0.8, 1e-6))
        self.assertTrue(Gf.IsClose(cmds.getAttr('%s.intensity' % nodePath),
            2, 1e-6))
        
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
        self.assertTrue(cmds.getAttr('%s.normalize' % nodePath) == 1)
   
    def _ValidateRoundtrip(self):
        cmds.file(new=True, force=True)
        # Import from USD.
        cmds.usdImport(file=self._usdFilePath, readAnimData=True, primPath='/')
        self._ValidateMayaDistantLight()
        self._ValidateMayaSpotLight()
        self._ValidateMayaPointLight()
        self._ValidateMayaAreaLight()
        
    def testExportLights(self):
        """
        Tests that Maya lights export as UsdLux schema USD prims
        correctly.
        """
        self._ValidateDistantLight()
        self._ValidatePointLight()
        self._ValidateSpotLight()
        self._ValidateAreaLight()

        self._ValidateRoundtrip()

if __name__ == '__main__':
    unittest.main(verbosity=2)
