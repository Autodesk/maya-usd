#!/pxrpythonsubst
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

from pxr import Usd
from pxr import UsdShade

from maya import cmds
from maya import standalone

import mayaUsd.lib as mayaUsdLib

import os
import unittest

import fixturesUtils


class testUsdExportUVSetMappings(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        suffix = ""
        if mayaUsdLib.WriteUtil.WriteMap1AsST():
            suffix += "ST"

        inputPath = fixturesUtils.setUpClass(__file__, suffix)

        mayaFile = os.path.join(inputPath, 'UsdExportUVSetMappingsTest',
            'UsdExportUVSetMappingsTest.ma')
        cmds.file(mayaFile, force=True, open=True)

        # Export to USD.
        usdFilePath = os.path.abspath('UsdExportUVSetMappingsTest.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo='UsdPreviewSurface',
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

    def testExportUVSetMappings(self):
        '''
        Tests that exporting multiple Maya planes with varying UV mappings
        setups results in USD data with material specializations:
        '''
        expected = [
            ("/pPlane1", "/blinn1SG_map1_map1_map1", "map1", "map1", "map1"),
            ("/pPlane2", "/blinn1SG", "st1", "st1", "st1"),
            ("/pPlane3", "/blinn1SG", "st1", "st1", "st1"),
            ("/pPlane4", "/blinn1SG_st2_st2_st2", "st2", "st2", "st2"),
            ("/pPlane5", "/blinn1SG_p5a_p5b_p5c", "p5a", "p5b", "p5c"),
            ("/pPlane6", "/blinn1SG_p62_p63_p61", "p62", "p63", "p61"),
            ("/pPlane7", "/blinn1SG_p7r_p7p_p7q", "p7r", "p7p", "p7q"),
        ]

        if mayaUsdLib.WriteUtil.WriteMap1AsST():
            # map1 renaming has almost no impact here. Getting better results
            # for that test would require something more radical, possibly
            #    uvSet[0] is always renamed to "st"
            # Which would reuse a single material for the first four planes.
            #
            # As currently implemented, the renaming affects only one material:
            expected[0] = ("/pPlane1", "/blinn1SG_st_st_st", "st", "st", "st")

        for mesh_name, mat_name, f1_name, f2_name, f3_name in expected:
            plane_prim = self._stage.GetPrimAtPath(mesh_name)
            binding_api = UsdShade.MaterialBindingAPI(plane_prim)
            mat = binding_api.ComputeBoundMaterial()[0]
            self.assertEqual(mat.GetPath(), mat_name)

            self.assertEqual(mat.GetInput("file1:varname").GetAttr().Get(), f1_name)
            self.assertEqual(mat.GetInput("file2:varname").GetAttr().Get(), f2_name)
            self.assertEqual(mat.GetInput("file3:varname").GetAttr().Get(), f3_name)

        # Initial code had a bug where a material with no UV mappings would
        # specialize itself. Make sure it stays fixed:
        plane_prim = self._stage.GetPrimAtPath("/pPlane8")
        binding_api = UsdShade.MaterialBindingAPI(plane_prim)
        mat = binding_api.ComputeBoundMaterial()[0]
        self.assertEqual(mat.GetPath(), "/pPlane8/Materials/blinn2SG")
        self.assertFalse(mat.GetPrim().HasAuthoredSpecializes())

if __name__ == '__main__':
    unittest.main(verbosity=2)
