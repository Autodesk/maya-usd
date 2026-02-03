#!/usr/bin/env mayapy
#
# Copyright 2018 Pixar
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
import sys
import unittest

from maya import cmds
from maya import standalone

from pxr import Usd, UsdGeom, Sdf

class testUsdExportPackage(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

        # Deprecated since version 3.3: assertNotRegexpMatches has been renamed
        # to assertNotRegex()
        if not hasattr(cls, "assertNotRegex"):
            cls.assertNotRegex = cls.assertNotRegexpMatches

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(os.path.abspath('PackageTest.ma'), open=True, force=True)
        self._existingTempFiles = set()

    def _AssertExpectedStage(self, stage, cardPngRefPath):
        self.assertTrue(stage)

        self.assertTrue(stage.GetPrimAtPath("/PackageTest"))
        self.assertTrue(stage.GetPrimAtPath("/PackageTest/MyAssembly"))
        self.assertTrue(stage.GetPrimAtPath(
                "/PackageTest/MyAssembly/BaseModel1"))
        self.assertTrue(stage.GetPrimAtPath(
                "/PackageTest/MyAssembly/BaseModel1/Geom/MyMesh"))
        self.assertTrue(stage.GetPrimAtPath(
                "/PackageTest/MyAssembly/BaseModel2"))
        self.assertTrue(stage.GetPrimAtPath(
                "/PackageTest/MyAssembly/BaseModel1/Geom/MyMesh"))

        prim = stage.GetPrimAtPath("/PackageTest/Sphere")
        self.assertTrue(prim)
        modelAPI = UsdGeom.ModelAPI(prim)
        self.assertEqual(modelAPI.GetModelCardTextureXNegAttr().Get().path,
                cardPngRefPath)

    def _listOfTempFiles(self, usdFilePath):
        return os.listdir(os.path.dirname(usdFilePath))
    
    def _recordExistingTempFiles(self, usdFilePath):
        self._existingTempFiles = set(self._listOfTempFiles(usdFilePath))

    def _AssertNoTempFiles(self, usdFilePath):
        lsDir = self._listOfTempFiles(usdFilePath)
        lsDir = [file for file in lsDir if file not in self._existingTempFiles]
        for item in lsDir:
            self.assertNotRegex(item, "tmp-.*\.usd.?")

    def testExport(self):
        '''Tests standard usdz package export.'''
        usdFile = os.path.abspath('MyAwesomePackage.usdz')

        self._recordExistingTempFiles(usdFile)

        cmds.usdExport(
                file=usdFile,
                mergeTransformAndShape=True,
                shadingMode='none')

        # Lets make sure that the root layer is the first file and that all
        # the references were localized ok.
        if Usd.GetVersion() < (0, 25, 8):
            zipFile = Usd.ZipFile.Open(usdFile)
        else:
            zipFile = Sdf.ZipFile.Open(usdFile)

        fileNames = zipFile.GetFileNames()

        # TODO: https://github.com/Autodesk/maya-usd/pull/555#discussion_r434170321
        if sys.version_info[0] < 3 :
            self.assertEqual(fileNames, [
                "MyAwesomePackage.usdc",
                "ReferenceModel.usda",
                "BaseModel.usda",
                "card.png"
            ])

        # Open the usdz file up to verify that everything exported properly.
        stage = Usd.Stage.Open(usdFile)
        self._AssertExpectedStage(stage, "./card.png")

        # Make sure there's no weird temp files sitting around.
        self._AssertNoTempFiles(usdFile)

    def testArKitCompatibility(self):
        '''Tests usdz package export with ARKit compatibility profile.'''
        usdFile = os.path.abspath('MyAwesomeArKitCompatibleFile.usdz')
        usdFileNoExt = os.path.abspath('MyAwesomeArKitCompatibleFile')

        self._recordExistingTempFiles(usdFile)

        # The usdExport command should automatically add "usdz" extension since
        # we're requestion appleArKit compatibility.
        cmds.usdExport(
                file=usdFileNoExt,
                mergeTransformAndShape=True,
                shadingMode='none',
                compatibility='appleArKit')

        # Lets make sure that the root layer is the first file and that all
        # the references were localized ok.
        # Note that the path of "card.png" in the usdz archive may have changed
        # because of the flattening step.
        if Usd.GetVersion() < (0, 25, 8):
            zipFile = Usd.ZipFile.Open(usdFile)
        else:
            zipFile = Sdf.ZipFile.Open(usdFile)

        fileNames = zipFile.GetFileNames()
        self.assertEqual(len(fileNames), 2)
        self.assertEqual(fileNames[0], "MyAwesomeArKitCompatibleFile.usdc")
        self.assertTrue(fileNames[1].endswith("card.png"))

        # Open the usdz file up to verify that everything exported properly.
        stage = Usd.Stage.Open(usdFile)
        self._AssertExpectedStage(stage, fileNames[-1])

        # Make sure there's no weird temp files sitting around.
        self._AssertNoTempFiles(usdFile)


if __name__ == '__main__':
    unittest.main(verbosity=2)
