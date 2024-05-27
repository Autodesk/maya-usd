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


class testUsdExportMaterialsOnly(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _export(self, exportMeshes=False, selection=None):
        '''
        Export the Maya scene with or without meshes and restricted to the selection or not.
        Then open the exported USD file.
        '''
        mayaFileName  = 'one-group.ma'
        self._mayaFile = os.path.join(self.inputPath, 'UsdExportMaterialScopeTest', mayaFileName)
        cmds.file(self._mayaFile, force=True, open=True)

        if selection:
            cmds.select(selection)

        # Export to USD.
        self._usdFilePath = os.path.abspath('MaterialsOnlyTest.usda')
        cmds.mayaUSDExport(
            mergeTransformAndShape=True,
            file=self._usdFilePath,
            shadingMode='useRegistry', 
            convertMaterialsTo=['UsdPreviewSurface'],
            materialsScopeName='Materials',
            defaultPrim='top_group',
            exportMaterials=True,
            selection=bool(selection),
            excludeExportTypes=[] if exportMeshes else ['Meshes'])

        self._stage = Usd.Stage.Open(self._usdFilePath)

    def _verifyMaterials(self, expectedMtlAndShaderPaths, expectPresent=True):
        '''
        Verify that the given materials and shaders are present or not.
        '''
        expectation = 'Expected to find' if expectPresent else 'Unexpected'
        for expectedPath, shader in expectedMtlAndShaderPaths:
            prim = self._stage.GetPrimAtPath(expectedPath)
            self.assertEqual(bool(prim), expectPresent,
                             '%s %s when exporting %s to %s' % (expectation, expectedPath, self._mayaFile, self._usdFilePath))
            if not expectPresent or not shader:
                continue
            shader = self._stage.GetPrimAtPath('%s/%s' % (expectedPath, shader))
            self.assertTrue(shader, 'Expected prim at %s to be a shader when exporting %s to %s' %  (expectedPath, self._mayaFile, self._usdFilePath))
            shader = UsdShade.Shader(shader)
            self.assertEqual(shader.GetIdAttr().Get(), 'UsdPreviewSurface')

    def _verifyMeshes(self, expectedMeshesPaths, expectPresent=True):
        '''
        Verify that the given meshes are present or not.
        '''
        expectation = 'Expected to find' if expectPresent else 'Unexpected'
        for expectedPath in expectedMeshesPaths:
            prim = self._stage.GetPrimAtPath(expectedPath)
            self.assertEqual(bool(prim), expectPresent,
                             '%s %s when exporting %s to %s' % (expectation, expectedPath, self._mayaFile, self._usdFilePath))

    def testStageOpens(self):
        '''
        Tests that the export can be done and that the USD stage can be opened successfully.
        '''
        self._export()
        self.assertTrue(self._stage)

    def testExportMeshesAndMaterials(self):
        '''
        Test that we can export meshes and materials and find both.
        When exporting like this, only assigned materials are exported.
        (Will change in the near future with a new export option.)
        '''
        self._export(exportMeshes=True)

        expectedMtlAndShaderPaths = [
            ('/top_group/Materials/standardSurface2SG', 'standardSurface2'),
            ('/top_group/Materials/standardSurface3SG', 'standardSurface3'),
            ('/top_group/Materials/standardSurface4SG', 'standardSurface4'),
            ('/top_group/Materials/standardSurface5SG', 'standardSurface5'),
        ]
        self._verifyMaterials(expectedMtlAndShaderPaths)

        expectedMeshPaths = [
            '/top_group/group1/pSphere1',
            '/top_group/group1/pCube1',
            '/top_group/group2/pCone1',
            '/top_group/pCylinder1',
        ]
        self._verifyMeshes(expectedMeshPaths, expectPresent=True)


    def testExportSelectedMeshAndMaterial(self):
        '''
        Test that we can export one selected mesh and another material and find both.
        '''
        self._export(exportMeshes=True, selection=['pSphere1', 'standardSurface4'])

        # Note: the material of the selected mesh is also exported.
        expectedMtlAndShaderPaths = [
            ('/top_group/Materials/standardSurface3SG', 'standardSurface3'),
            ('/top_group/Materials/standardSurface4SG', 'standardSurface4'),
        ]
        self._verifyMaterials(expectedMtlAndShaderPaths)

        excludedMtlAndShaderPaths = [
            ('/top_group/Materials/standardSurface2SG', None),
            ('/top_group/Materials/standardSurface5SG', None),
        ]
        self._verifyMaterials(excludedMtlAndShaderPaths, expectPresent=False)

        expectedMeshPaths = [
            '/top_group/group1/pSphere1',
        ]
        self._verifyMeshes(expectedMeshPaths, expectPresent=True)

        excludedMeshPaths = [
            '/top_group/group1/pCube1',
            '/top_group/group2/pCone1',
            '/top_group/pCylinder1',
        ]
        self._verifyMeshes(excludedMeshPaths, expectPresent=False)


    def testExportMaterialsOnly(self):
        '''
        Test that we can export only materials.
        In this case, all materials are exported, even if unused.
        No mesh should be found in the exported file.
        '''
        self._export(exportMeshes=False)

        expectedMtlAndShaderPaths = [
            ('/top_group/Materials/initialShadingGroup', None),
            ('/top_group/Materials/standardSurface2SG', 'standardSurface2'),
            ('/top_group/Materials/standardSurface3SG', 'standardSurface3'),
            ('/top_group/Materials/standardSurface4SG', 'standardSurface4'),
            ('/top_group/Materials/standardSurface5SG', 'standardSurface5'),
        ]
        self._verifyMaterials(expectedMtlAndShaderPaths)

        excludedMeshPaths = [
            '/top_group/group1/pSphere1',
            '/top_group/group1/pCube1',
            '/top_group/group2/pCone1',
            '/top_group/pCylinder1',
        ]
        self._verifyMeshes(excludedMeshPaths, expectPresent=False)

    def testExportSelectedMaterialsOnly(self):
        '''
        Test that we can export only the selected materials.
        In this case, all materials are exported, even if unused.
        No mesh should be found in the exported file.
        '''
        self._export(exportMeshes=False, selection=['standardSurface2', 'standardSurface4'])

        expectedMtlAndShaderPaths = [
            ('/top_group/Materials/standardSurface2SG', 'standardSurface2'),
            ('/top_group/Materials/standardSurface4SG', 'standardSurface4'),
        ]
        self._verifyMaterials(expectedMtlAndShaderPaths)

        excludedMtlAndShaderPaths = [
            ('/top_group/Materials/initialShadingGroup', None),
            ('/top_group/Materials/standardSurface3SG', None),
            ('/top_group/Materials/standardSurface5SG', None),
        ]
        self._verifyMaterials(excludedMtlAndShaderPaths, expectPresent=False)

        excludedMeshPaths = [
            '/top_group/group1/pSphere1',
            '/top_group/group1/pCube1',
            '/top_group/group2/pCone1',
            '/top_group/pCylinder1',
        ]
        self._verifyMeshes(excludedMeshPaths, expectPresent=False)


if __name__ == '__main__':
    unittest.main(verbosity=2)
