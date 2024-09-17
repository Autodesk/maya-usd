#!/usr/bin/env mayapy
#
# Copyright 2024 Autodesk
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
from pxr import UsdShade
from pxr import Sdr

from maya import cmds
from maya import standalone

import os
import unittest

import fixturesUtils


class testUsdExportMaterialScopeLegacy(unittest.TestCase):

    _oneGroupMayaFileName  = 'one-group.ma'
    _twoGroupsMayaFileName = 'two-groups.ma'

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _export(self, mayaFileName, defaultPrim=''):
        self._mayaFile = os.path.join(self.inputPath, 'UsdExportMaterialScopeTest', mayaFileName)
        cmds.file(self._mayaFile, force=True, open=True)

        # Export to USD.
        self._usdFilePath = os.path.abspath('MaterialScopeTest.usda')
        cmds.mayaUSDExport(
            mergeTransformAndShape=True,
            file=self._usdFilePath,
            shadingMode='useRegistry', 
            convertMaterialsTo=['UsdPreviewSurface'],
            materialsScopeName='Materials',
            legacyMaterialScope=True,
            defaultPrim=defaultPrim)

        self._stage = Usd.Stage.Open(self._usdFilePath)

    def _verifyMaterials(self, expectedMtlPaths):
        for expectedPath in expectedMtlPaths:
            prim = self._stage.GetPrimAtPath(expectedPath)
            self.assertTrue(prim, 'Expected to find %s when exporting %s to %s' % (expectedPath, self._mayaFile, self._usdFilePath))
            shader = UsdShade.Shader(prim)
            self.assertTrue(shader, 'Expected prim at %s to be a shader when exporting %s to %s' %  (expectedPath, self._mayaFile, self._usdFilePath))
            self.assertEqual(shader.GetIdAttr().Get(), 'UsdPreviewSurface')

    def testStageOpens(self):
        '''
        Tests that the export can be done and that the USD stage can be opened successfully.
        '''
        self._export(self._oneGroupMayaFileName)
        self.assertTrue(self._stage)

    def testMaterialScopeOneGroupNoDefaultPrim(self):
        self._export(self._oneGroupMayaFileName)

        expectedMtlPaths = [
            '/top_group/Materials/standardSurface2SG/standardSurface2',
            '/top_group/Materials/standardSurface3SG/standardSurface3',
            '/top_group/Materials/standardSurface4SG/standardSurface4',
            '/top_group/Materials/standardSurface5SG/standardSurface5',
        ]
        self._verifyMaterials(expectedMtlPaths)

    def testMaterialScopeOneGroupWithDefaultPrim(self):
        self._export(self._oneGroupMayaFileName, defaultPrim='top_group')

        expectedMtlPaths = [
            '/top_group/Materials/standardSurface2SG/standardSurface2',
            '/top_group/Materials/standardSurface3SG/standardSurface3',
            '/top_group/Materials/standardSurface4SG/standardSurface4',
            '/top_group/Materials/standardSurface5SG/standardSurface5',
        ]
        self._verifyMaterials(expectedMtlPaths)

    def testMaterialScopeTwoGroupsNoDefaultPrim(self):
        self._export(self._twoGroupsMayaFileName)

        expectedMtlPaths = [
            '/group1/Materials/standardSurface2SG/standardSurface2',
            '/group1/Materials/standardSurface3SG/standardSurface3',
            '/group2/Materials/standardSurface4SG/standardSurface4',
            '/pCylinder1/Materials/standardSurface5SG/standardSurface5',
        ]
        self._verifyMaterials(expectedMtlPaths)

    def testMaterialScopeTwoGroupsWithDefaultPrim(self):
        self._export(self._twoGroupsMayaFileName, defaultPrim='group1')

        expectedMtlPaths = [
            '/group1/Materials/standardSurface2SG/standardSurface2',
            '/group1/Materials/standardSurface3SG/standardSurface3',
            '/group2/Materials/standardSurface4SG/standardSurface4',
            '/pCylinder1/Materials/standardSurface5SG/standardSurface5',
        ]
        self._verifyMaterials(expectedMtlPaths)


if __name__ == '__main__':
    unittest.main(verbosity=2)
