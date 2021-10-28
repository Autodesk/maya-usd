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

from pxr import Usd
from pxr import UsdShade

from maya import cmds
from maya import standalone

import mayaUsd.lib as mayaUsdLib

import os
import unittest

import fixturesUtils


class testUsdExportMaterialX(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        mayaFile = os.path.join(inputPath, 'UsdExportMultiMaterialTest',
            'MultiMaterialTest.ma')
        cmds.file(mayaFile, force=True, open=True)

        # Export to USD.
        usdFilePath = os.path.abspath('MultiMaterialTest.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', 
            convertMaterialsTo=['MaterialX', 'UsdPreviewSurface', 'rendermanForMaya'],
            materialsScopeName='Materials')

        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        '''
        Tests that the USD stage was opened successfully.
        '''
        self.assertTrue(self._stage)

    def testExportedUsdShadeNodeTypes(self):
        '''
        Tests that all node ids are what we expect:
        '''

        base_path = "/pCube{0}/Materials/{1}SG/{2}/{1}"
        to_test = [
            # pCube1 has standard_surface, known to UsdPreviewSurface and MaterialX
            (1, "standardSurface2", "UsdPreviewSurface", "UsdPreviewSurface"),
            (1, "standardSurface2", "MaterialX", "ND_standard_surface_surfaceshader"),
            # pCube2 has lambert, known to UsdPreviewSurface and RfM
            (2, "lambert2", "UsdPreviewSurface", "UsdPreviewSurface"),
            (2, "lambert2", "rendermanForMaya", "PxrMayaLambert"),
            # pCube3 has UsdPreviewSurface, known to UsdPreviewSurface and MaterialX
            (3, "usdPreviewSurface1", "UsdPreviewSurface", "UsdPreviewSurface"),
            (3, "usdPreviewSurface1", "MaterialX", "ND_UsdPreviewSurface_surfaceshader"),
        ]

        for prim_idx, shd_name, shd_scope, id_attr in to_test:
            prim_path = base_path.format(prim_idx, shd_name, shd_scope)

            prim = self._stage.GetPrimAtPath(prim_path)
            self.assertTrue(prim, prim_path)
            shader = UsdShade.Shader(prim)
            self.assertTrue(shader, prim_path)
            self.assertEqual(shader.GetIdAttr().Get(), id_attr)

if __name__ == '__main__':
    unittest.main(verbosity=2)
