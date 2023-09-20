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

from maya import cmds
from maya import standalone

from pxr import Gf, Sdf, Usd, UsdGeom, UsdSkel, Vt

import fixturesUtils

class testUsdExportTypes(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

        cls.testFile = os.path.join(cls.inputPath, "UsdExportTypeTest", "UsdExportTypeTest.ma")
        cmds.file(cls.testFile, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _ValidateMesh(self):
        cmds.file(self.testFile, force=True, open=True)
        usdFile = os.path.abspath('UsdExportMeshOnly.usda')

        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', excludeExportTypes=['Cameras','Lights'])
        stage = Usd.Stage.Open(usdFile)
    
        meshPrims = ['/pSphere1','/pCube1']
        for m in meshPrims:
            meshPrim = stage.GetPrimAtPath(m)
            self.assertTrue(meshPrim)

    def _ValidateLight(self):
        cmds.file(self.testFile, force=True, open=True)
        usdFile = os.path.abspath('UsdExportLightOnly.usda')

        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', excludeExportTypes=['Cameras','Meshes'])
        stage = Usd.Stage.Open(usdFile)
    
        lightPrims = ['/directionalLight1','/pointLight1','/areaLight1','/spotLight1']
        for l in lightPrims:
            lightPrim = stage.GetPrimAtPath(l)
            self.assertTrue(lightPrim)

    def _ValidateCamera(self):
        cmds.file(self.testFile, force=True, open=True)
        usdFile = os.path.abspath('UsdExportCameraOnly.usda')

        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', excludeExportTypes=['Lights','Meshes'])
        stage = Usd.Stage.Open(usdFile)
    
        camera = '/camera1'
        cameraPrim = stage.GetPrimAtPath(camera)
        self.assertTrue(cameraPrim)

    def _ValidateMeshCamera(self):
        cmds.file(self.testFile, force=True, open=True)
        usdFile = os.path.abspath('UsdExportMeshCamera.usda')

        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', excludeExportTypes=['Lights'])
        stage = Usd.Stage.Open(usdFile)
    
        camera = '/camera1'
        cameraPrim = stage.GetPrimAtPath(camera)
        self.assertTrue(cameraPrim)

        meshPrims = ['/pSphere1','/pCube1']
        for m in meshPrims:
            meshPrim = stage.GetPrimAtPath(m)
            self.assertTrue(meshPrim)

    def _ValidateMeshLight(self):
        cmds.file(self.testFile, force=True, open=True)
        usdFile = os.path.abspath('UsdExportMeshCamera.usda')

        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', excludeExportTypes=['Cameras'])
        stage = Usd.Stage.Open(usdFile)
    
        lightPrims = ['/directionalLight1','/pointLight1','/areaLight1','/spotLight1']
        for l in lightPrims:
            lightPrim = stage.GetPrimAtPath(l)
            self.assertTrue(lightPrim)

        meshPrims = ['/pSphere1','/pCube1']
        for m in meshPrims:
            meshPrim = stage.GetPrimAtPath(m)
            self.assertTrue(meshPrim)
    
    def testExportTypes(self):
        self._ValidateMesh()
        self._ValidateLight()
        self._ValidateCamera()
        self._ValidateMeshCamera()
        self._ValidateMeshLight()

if __name__ == '__main__':
    unittest.main(verbosity=2)
