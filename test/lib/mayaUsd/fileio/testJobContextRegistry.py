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

import mayaUsd.lib as mayaUsdLib

from pxr import UsdGeom
from pxr import Usd

from maya import cmds
import maya.api.OpenMaya as OpenMaya
from maya import standalone

import fixturesUtils, os

import unittest

def importEnablerFn():
    print("importEnablerFn called")
    return {}

def exportEnablerFn():
    print("exportEnablerFn called")
    return {}

class testJobContextRegistry(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        pass

    def testRegistration(self):
        mayaUsdLib.JobContextRegistry.RegisterImportJobContext("TestImport", "My import test", "My simple import registration test", importEnablerFn)
        mayaUsdLib.JobContextRegistry.RegisterExportJobContext("TestExport", "My export test", "My simple export registration test", exportEnablerFn)
        print(mayaUsdLib.JobContextRegistry.GetJobContextInfo("TestImport"))
        print(mayaUsdLib.JobContextRegistry.GetJobContextInfo("TestExport"))

if __name__ == '__main__':
    unittest.main(verbosity=2)
