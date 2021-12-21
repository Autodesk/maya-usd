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

        mayaFile = os.path.join(inputPath, 'UsdExportMaterialXTest',
            'StandardSurfaceTextured.ma')
        cmds.file(mayaFile, force=True, open=True)

        # Export to USD.
        usdFilePath = os.path.abspath('UsdExportMaterialXTest.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['MaterialX'],
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

    def compareValue(self, shader, name, value):
        """Test that a named attribute has the expected value"""
        attr = shader.GetInput(name)
        self.assertTrue(attr)
        return attr.GetAttr().Get() == value

    def testExportTexturedMaterialXpPlane1(self):
        '''
        Tests that pPlane1 exported as planned:

        this plane is a basic RGB texture without any customizations.
        '''

        # Exploring this path:
        base_path = "/pPlane1/Materials/standardSurface2SG"

        mesh_prim = self._stage.GetPrimAtPath('/pPlane1')
        self.assertTrue(mesh_prim)

        # Validate the Material prim bound to the Mesh prim.
        self.assertTrue(mesh_prim.HasAPI(UsdShade.MaterialBindingAPI))
        mat_binding = UsdShade.MaterialBindingAPI(mesh_prim)
        mat = mat_binding.ComputeBoundMaterial("mtlx")[0]
        self.assertTrue(mat)
        material_path = mat.GetPath().pathString
        self.assertEqual(material_path, base_path)

        # Needs a resolved inputs:file1:varnameStr attribute:
        self.assertEqual(mat.GetInput("file1:varnameStr").GetAttr().Get(), "st")

        # Needs a MaterialX surface source:
        shader = mat.ComputeSurfaceSource("mtlx")[0]
        self.assertTrue(shader)

        # Which is a standard surface:
        self.assertEqual(shader.GetIdAttr().Get(),
                         "ND_standard_surface_surfaceshader")

        # With a connected file texture on base_color going to baseColor on the
        # nodegraph:
        attr = shader.GetInput('base_color')
        self.assertTrue(attr)
        cnxTuple = attr.GetConnectedSource()
        self.assertTrue(cnxTuple)

        ng_path = base_path + '/MayaNG_standardSurface2SG'
        ng = UsdShade.NodeGraph(cnxTuple[0])
        self.assertEqual(ng.GetPath(), ng_path)
        self.assertEqual(cnxTuple[1], "baseColor")

        # Should have an outputs connected to a file node:
        attr = ng.GetOutput('baseColor')
        self.assertTrue(attr)
        cnxTuple = attr.GetConnectedSource()
        self.assertTrue(cnxTuple)

        # Which is a color3 image:
        shader = UsdShade.Shader(cnxTuple[0])
        self.assertEqual(shader.GetIdAttr().Get(), "ND_image_color3")
        self.assertEqual(shader.GetPath(), ng_path + "/file1")

        # Check a few values:
        self.assertTrue(self.compareValue(shader, "uaddressmode", "periodic"))
        self.assertTrue(self.compareValue(shader, "default", (0.5, 0.5, 0.5)))

        # Which is itself connected to a primvar reader:
        attr = shader.GetInput('texcoord')
        self.assertTrue(attr)
        cnxTuple = attr.GetConnectedSource()
        self.assertTrue(cnxTuple)

        # Which is a geompropvalue node:
        shader = UsdShade.Shader(cnxTuple[0])
        self.assertEqual(shader.GetIdAttr().Get(), "ND_geompropvalue_vector2")
        self.assertEqual(shader.GetPath(),
                         ng_path + "/place2dTexture1")

    def testExportTexturedMaterialXNodeTypes(self):
        '''
        Tests all node ids that are expected:
        '''

        base_path = "/pPlane{0}/Materials/standardSurface{1}SG/MayaNG_standardSurface{1}SG/{2}"
        to_test = [
            (7, 8, "file7", "ND_image_float"),
            (6, 7, "file6", "ND_image_vector2"),
            (1, 2, "file1", "ND_image_color3"),
            (4, 5, "file4", "ND_image_color4"),

            (1, 2, "place2dTexture1", "ND_geompropvalue_vector2"),

            (4, 5, "MayaSwizzle_file4_rgb", "ND_swizzle_color4_color3"),
            (6, 7, "MayaSwizzle_file6_xxx", "ND_swizzle_vector2_color3"),
            (19, 21, "MayaSwizzle_file20_x", "ND_swizzle_vector2_float"),
            (7, 8, "MayaSwizzle_file7_rrr", "ND_swizzle_float_color3"),
            (8, 9, "MayaSwizzle_file8_r", "ND_swizzle_color4_float"),
            (13, 14, "MayaSwizzle_file13_g", "ND_swizzle_color3_float"),

            (27, 20, "MayaLuminance_file27", "ND_luminance_color3"),
            (12, 13, "MayaLuminance_file12", "ND_luminance_color4"),

            (14, 15, "MayaConvert_file14_color3f_float3", 
             "ND_convert_color3_vector3"),
            (15, 16, "MayaNormalMap_standardSurface16_normalCamera",
             "ND_normalmap"),
        ]

        for prim_idx, sg_idx, node_name, id_attr in to_test:
            prim_path = base_path.format(prim_idx, sg_idx, node_name)

            prim = self._stage.GetPrimAtPath(prim_path)
            self.assertTrue(prim, prim_path)
            shader = UsdShade.Shader(prim)
            self.assertTrue(shader, prim_path)
            self.assertEqual(shader.GetIdAttr().Get(), id_attr, id_attr)


if __name__ == '__main__':
    unittest.main(verbosity=2)
