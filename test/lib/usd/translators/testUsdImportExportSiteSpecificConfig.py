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
    CalledAsExpected = False
    def __init__(self, factoryContext, *args, **kwargs):
        super(TestExportChaser, self).__init__(factoryContext, *args, **kwargs)
        TestExportChaser.CalledAsExpected = factoryContext.GetJobArgs().exportDisplayColor
        if "auto" != factoryContext.GetJobArgs().exportSkin:
            TestExportChaser.CalledAsExpected = False
        if "TestExportChaserFromSiteSpecific" not in factoryContext.GetJobArgs().chaserNames:
            TestExportChaser.CalledAsExpected = False
        if "TestExportChaserFromSiteSpecific" not in factoryContext.GetJobArgs().allChaserArgs or\
            "frob" not in factoryContext.GetJobArgs().allChaserArgs["TestExportChaserFromSiteSpecific"]:
            TestExportChaser.CalledAsExpected = False

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
        cmds.mayaUSDExport(file=usd_file_path)

        # Import scene, which should use site-specific settings and trigger the chaser:
        cmds.mayaUSDImport(file=usd_file_path)

        self.assertTrue(TestExportChaser.CalledAsExpected)
        self.assertTrue(TestImportChaser.CalledAsExpected)


if __name__ == '__main__':
    unittest.main(verbosity=2)
