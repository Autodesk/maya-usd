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

from pxr import Usd
from pxr import UsdGeom

from maya import cmds
from maya import standalone

import os
import unittest

import fixturesUtils


class testUsdMayaAdaptorGeom(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

        usdFile = os.path.join(cls.inputPath, 'UsdMayaAdaptorGeomTest',
            'UsdAttrs.usda')
        cmds.usdImport(file=usdFile, shadingMode=[["none", "default"], ])

    def testImportImageable(self):
        """
        Tests that UsdGeomImageable.purpose is properly imported.
        """
        # Testing for the different purpose attributes
        self.assertEqual(cmds.getAttr('pCube1.USD_ATTR_purpose'), 'default')
        self.assertEqual(cmds.getAttr('pCube2.USD_ATTR_purpose'), 'render')
        self.assertEqual(cmds.getAttr('pCube3.USD_ATTR_purpose'), 'proxy')

        # pCube4 does not have a purpose attribute
        self.assertFalse(cmds.objExists('pCube4.USD_ATTR_purpose'))
        self.assertFalse(cmds.objExists('pCube4.USD_purpose')) # alias
    
    def testExportImageable(self):
        """
        Test that UsdGeomImageable.purpose is properly exported.
        """
        newUsdFilePath = os.path.abspath('UsdAttrsNew.usda')
        cmds.usdExport(file=newUsdFilePath, shadingMode='none')
        newUsdStage = Usd.Stage.Open(newUsdFilePath)
        
        # Testing the exported purpose attributes
        geom1 = UsdGeom.Imageable(newUsdStage.GetPrimAtPath('/World/pCube1'))
        self.assertEqual(geom1.GetPurposeAttr().Get(), 'default')
        geom2 = UsdGeom.Imageable(newUsdStage.GetPrimAtPath('/World/pCube2'))
        self.assertEqual(geom2.GetPurposeAttr().Get(), 'render')
        geom3 = UsdGeom.Imageable(newUsdStage.GetPrimAtPath('/World/pCube3'))
        self.assertEqual(geom3.GetPurposeAttr().Get(), 'proxy')

        # Testing that there is no authored attribute
        geom4 = UsdGeom.Imageable(newUsdStage.GetPrimAtPath('/World/pCube4'))
        self.assertFalse(geom4.GetPurposeAttr().HasAuthoredValue())

if __name__ == '__main__':
    unittest.main(verbosity=2)
