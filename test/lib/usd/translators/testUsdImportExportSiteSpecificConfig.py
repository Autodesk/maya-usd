#!/usr/bin/env mayapy
#
# Copyright 2022 Autodesk
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

# Sample translated from C++ from 
#   test/lib/usd/plugin/infoImportChaser.cpp and 
#   test/lib/usd/translators/testUsdImportChaser.py

import mayaUsd.lib as mayaUsdLib

from pxr import UsdGeom
from pxr import Usd

from maya import cmds
import maya.api.OpenMaya as OpenMaya
from maya import standalone

import fixturesUtils, os

import unittest

class TestExportChaser(mayaUsdLib.ExportChaser):
    ValidExportDisplayColor = False
    ValidExportSkin = False
    ValidChaserNames = False
    ValidChaserArgs = False
    ValidDagToUsdMap = False
    def __init__(self, factoryContext, *args, **kwargs):
        super(TestExportChaser, self).__init__(factoryContext, *args, **kwargs)
        TestExportChaser.ValidExportDisplayColor = factoryContext.GetJobArgs().exportDisplayColor
        TestExportChaser.ValidExportSkin = ("auto" == factoryContext.GetJobArgs().exportSkin)
        TestExportChaser.ValidChaserNames = "TestExportChaserFromSiteSpecific" in factoryContext.GetJobArgs().chaserNames
        TestExportChaser.ValidChaserArgs = "TestExportChaserFromSiteSpecific" in factoryContext.GetJobArgs().allChaserArgs and\
            "frob" in factoryContext.GetJobArgs().allChaserArgs["TestExportChaserFromSiteSpecific"]
        # Note: the DAG-to-USD map contains MDagPath and SdfPath. To avoid the complexity of building a MDagPath just
        #       for the test, which would require getting the Maya node and getting a MDagPath from it, we convert
        #       both to strings and use strings instead.
        d2u = factoryContext.GetDagToUsdMap()
        dagToUsdMap = { str(item.key()) : str(item.data()) for item in d2u }
        TestExportChaser.ValidDagToUsdMap = "pCube1" in dagToUsdMap and "pCubeShape1" in dagToUsdMap

class TestImportChaser(mayaUsdLib.ImportChaser):
    CalledAsExpected = False
    def __init__(self, factoryContext, *args, **kwargs):
        super(TestImportChaser, self).__init__(factoryContext, *args, **kwargs)
        TestImportChaser.CalledAsExpected = factoryContext.GetJobArgs().useAsAnimationCache
        if "phong" != factoryContext.GetJobArgs().preferredMaterial:
            TestImportChaser.CalledAsExpected = False
        if "TestImportChaserFromSiteSpecific" not in factoryContext.GetJobArgs().chaserNames:
            TestImportChaser.CalledAsExpected = False
        if "TestImportChaserFromSiteSpecific" not in factoryContext.GetJobArgs().allChaserArgs or\
            "frob" not in factoryContext.GetJobArgs().allChaserArgs["TestImportChaserFromSiteSpecific"]:
            TestImportChaser.CalledAsExpected = False


class testSiteSpecificConfig(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testChasers(self):
        mayaUsdLib.ImportChaser.Register(TestImportChaser, "TestImportChaserFromSiteSpecific")
        mayaUsdLib.ExportChaser.Register(TestExportChaser, "TestExportChaserFromSiteSpecific")

        cmds.polyCube()

        # Export scene, which should use site-specific settings and trigger the chaser:
        usd_file_path = os.path.join(self.temp_dir, "SiteSpecificFile.usda")
        cmds.mayaUSDExport(file=usd_file_path, defaultPrim='pCube1')

        # Import scene, which should use site-specific settings and trigger the chaser:
        cmds.mayaUSDImport(file=usd_file_path)

        self.assertTrue(TestExportChaser.ValidChaserArgs)
        self.assertTrue(TestExportChaser.ValidChaserNames)
        self.assertTrue(TestExportChaser.ValidDagToUsdMap)
        self.assertTrue(TestExportChaser.ValidExportDisplayColor)
        self.assertTrue(TestExportChaser.ValidExportSkin)
        self.assertTrue(TestImportChaser.CalledAsExpected)


if __name__ == '__main__':
    unittest.main(verbosity=2)
