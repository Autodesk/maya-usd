#!/usr/bin/env mayapy
#
# Copyright 2020 Autodesk
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

from pxr import Sdf, Usd, Vt

import mayaUsd.schemas as mayaUsdSchemas
import mayaUsd.lib as mayaUsdLib

from maya import cmds
from maya import standalone

import os
import unittest

import fixturesUtils

class testUsdImportMayaReference(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.readOnlySetUpClass(__file__)

        usdFile = os.path.join(inputPath, "UsdImportMayaReferenceTest", "MayaReference.usda")
        cmds.usdImport(file=usdFile, shadingMode=[['none', 'default'], ])

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testImport(self):
        mayaReference = 'rig:cubeRig'
        self.assertTrue(cmds.objExists(mayaReference))

if __name__ == '__main__':
    unittest.main(verbosity=2)
