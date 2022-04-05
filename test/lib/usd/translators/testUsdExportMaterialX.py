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
from pxr import Sdf
from pxr import UsdShade

from maya import cmds
from maya import standalone

import mayaUsd.lib as mayaUsdLib

import maya.api.OpenMaya as om

import os
import unittest

import fixturesUtils

class mxCompositeExportTest(mayaUsdLib.ShaderWriter):
    _nameConversion = {
        "colorA" : "bg",
        "colorB" : "fg",
        "factor" : "mix",
    }
    # All untested excepted for "mix":
    _operationEquivalent = {
        0: "ND_plus_color3",
        1: "ND_minus_color3",
        2: "ND_mix_color3",
        3: "UNKNOWN_multiply_color3",
        4: "ND_screen_color3",
        5: "ND_overlay_color3",
        6: "ND_difference_color3",
        7: "ND_dodge_color3",
        8: "ND_burn_color3",
    }
    _ShadingModes = []

    @classmethod
    def CanExport(cls, exportArgs):
        cls._AllMaterialConversions = exportArgs.allMaterialConversions
        if exportArgs.convertMaterialsTo == "MaterialX":
            return mayaUsdLib.ShaderWriter.ContextSupport.Supported
        else:
            return mayaUsdLib.ShaderWriter.ContextSupport.Unsupported

    def Write(self, usdTime):
        """
            Writes the Maya colorComposite node to MaterialX
        """
        try:
            # Not handling animation (so does much of the shader export code)
            if usdTime != Usd.TimeCode.Default():
                return

            # Everything must be added in the material node graph:
            materialPath = self.GetUsdPath().GetParentPath()
            ngName = "MayaNG_" + materialPath.name
            ngPath = materialPath.AppendChild(ngName)

            mayaNode = om.MFnDependencyNode(self.GetMayaObject())
            # Normally we sanitize the name to remove namespace ":"
            nodePath = ngPath.AppendChild(mayaNode.name())

            nodeShader = UsdShade.Shader.Define(self.GetUsdStage(), nodePath)
            self._SetUsdPrim(nodeShader.GetPrim())

            mayaPlug = mayaNode.findPlug("operation", True)
            mxOperation = mxCompositeExportTest._operationEquivalent[mayaPlug.asInt()]
            nodeShader.CreateIdAttr(mxOperation)
            nodeShader.CreateOutput("out", Sdf.ValueTypeNames.Color3f)

            for mayaName, usdName in mxCompositeExportTest._nameConversion.items():
                mayaPlug = mayaNode.findPlug(mayaName, True)

                if not mayaUsdLib.Util.IsAuthored(mayaPlug):
                    continue

                valueTypeName = Sdf.ValueTypeNames.Color3f
                if usdName == "mix":
                    valueTypeName = Sdf.ValueTypeNames.Float

                usdInput = nodeShader.CreateInput(usdName, valueTypeName)

                if not mayaPlug.isDestination:
                    mayaUsdLib.WriteUtil.SetUsdAttr(
                        mayaPlug, usdInput, usdTime, self._GetSparseValueWriter())

        except Exception as e:
            # Quite useful to debug errors in a Python callback
            print('Write() - Error: %s' % str(e))

    def GetShadingAttributeNameForMayaAttrName(self, mayaAttrName):
        if mayaAttrName == "outColor":
            return UsdShade.Utils.GetFullName("out", UsdShade.AttributeType.Output)
        else:
            retVal = mxCompositeExportTest._nameConversion.get(mayaAttrName, "")
            return UsdShade.Utils.GetFullName(retVal, UsdShade.AttributeType.Input)


class testUsdExportMaterialX(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls._inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

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
        cmds.file(f=True, new=True)

        mayaFile = os.path.join(self._inputPath, 'UsdExportMaterialXTest',
            'StandardSurfaceTextured.ma')
        cmds.file(mayaFile, force=True, open=True)

        # Export to USD.
        usdFilePath = os.path.abspath('UsdExportMaterialXTest.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['MaterialX'],
            materialsScopeName='Materials')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        # Exploring this path:
        base_path = "/pPlane1/Materials/standardSurface2SG"

        mesh_prim = stage.GetPrimAtPath('/pPlane1')
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

        # Should have an outputs connected to a fileTexture node:
        attr = ng.GetOutput('baseColor')
        self.assertTrue(attr)
        cnxTuple = attr.GetConnectedSource()
        self.assertTrue(cnxTuple)

        # Which is an image post-processor:
        shader = UsdShade.Shader(cnxTuple[0])
        self.assertEqual(shader.GetIdAttr().Get(), "MayaND_fileTexture_color3")
        self.assertTrue(self.compareValue(shader, "defaultColor", (0.5, 0.5, 0.5)))
        attr = shader.GetInput('inColor')
        self.assertTrue(attr)
        cnxTuple = attr.GetConnectedSource()
        self.assertTrue(cnxTuple)

        # Which is a color3 image:
        shader = UsdShade.Shader(cnxTuple[0])
        self.assertEqual(shader.GetIdAttr().Get(), "ND_image_color3")
        self.assertEqual(shader.GetPath(), ng_path + "/file1")

        # Check a few values:
        self.assertTrue(self.compareValue(shader, "uaddressmode", "periodic"))

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

        base_path = "/pPlane{0}/Materials/standardSurface{1}SG/MayaNG_standardSurface{1}SG/{2}"
        to_test = [
            (7, 8, "file7", "ND_image_float"),
            (6, 7, "file6", "ND_image_vector2"),
            (1, 2, "file1", "ND_image_color3"),
            (4, 5, "file4", "ND_image_color4"),

            (7, 8, "file7_MayafileTexture", "MayaND_fileTexture_float"),
            (6, 7, "file6_MayafileTexture", "MayaND_fileTexture_vector2"),
            (1, 2, "file1_MayafileTexture", "MayaND_fileTexture_color3"),
            (4, 5, "file4_MayafileTexture", "MayaND_fileTexture_color4"),

            (1, 2, "place2dTexture1", "ND_geompropvalue_vector2"),

            (4, 5, "MayaSwizzle_file4_MayafileTexture_rgb", "ND_swizzle_color4_color3"),
            (6, 7, "MayaSwizzle_file6_MayafileTexture_xyy", "ND_swizzle_vector2_color3"),
            (19, 21, "MayaSwizzle_file20_MayafileTexture_x", "ND_swizzle_vector2_float"),
            (7, 8, "MayaSwizzle_file7_MayafileTexture_xxx", "ND_swizzle_float_color3"),
            (8, 9, "MayaSwizzle_file8_MayafileTexture_r", "ND_swizzle_color4_float"),
            (13, 14, "MayaSwizzle_file13_MayafileTexture_g", "ND_swizzle_color3_float"),
            (14, 15, "MayaSwizzle_file14_MayafileTexture_rgb", "ND_swizzle_color3_vector3"),

            (27, 20, "MayaLuminance_file27", "ND_luminance_color3"),
            (12, 13, "MayaLuminance_file12", "ND_luminance_color4"),

            (15, 16, "MayaNormalMap_standardSurface16_normalCamera",
             "ND_normalmap"),
        ]

        for prim_idx, sg_idx, node_name, id_attr in to_test:
            prim_path = base_path.format(prim_idx, sg_idx, node_name)

            prim = stage.GetPrimAtPath(prim_path)
            self.assertTrue(prim, prim_path)
            shader = UsdShade.Shader(prim)
            self.assertTrue(shader, prim_path)
            self.assertEqual(shader.GetIdAttr().Get(), id_attr, id_attr)

    def testPythonCustomShaderExporter(self):
        '''
        Add a custom exporter to the mix and see if it can export a compositing node.
        '''
        cmds.file(f=True, new=True)

        # Register our new exporter:
        mayaUsdLib.ShaderWriter.Register(mxCompositeExportTest, "colorComposite")

        mayaFile = os.path.join(self._inputPath, 'UsdExportMaterialXTest',
            'MaterialX_decal.ma')
        cmds.file(mayaFile, force=True, open=True)

        # Export to USD.
        usdFilePath = os.path.abspath('MaterialX_decal.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['MaterialX'],
            materialsScopeName='Materials')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        # We should have a nice colorComposite1 node in the graph curtesy of the custom exporter:
        prim = stage.GetPrimAtPath("/pPlane1/Materials/standardSurface2SG/MayaNG_standardSurface2SG/colorComposite1")
        self.assertTrue(prim)
        shader = UsdShade.Shader(prim)
        self.assertTrue(shader)
        self.assertEqual(shader.GetIdAttr().Get(), "ND_mix_color3")
        input = shader.GetInput("fg")
        self.assertEqual(input.Get(), (1,1,0))
        input = shader.GetInput("bg")
        cnxTuple = input.GetConnectedSource()
        self.assertTrue(cnxTuple)
        self.assertEqual(cnxTuple[0].GetPrim().GetName(), "file1_MayafileTexture")
        input = shader.GetInput("mix")
        cnxTuple = input.GetConnectedSource()
        self.assertTrue(cnxTuple)
        self.assertEqual(cnxTuple[0].GetPrim().GetName(), "MayaSwizzle_file2_MayafileTexture_a")
        self.assertTrue("MaterialX" in mxCompositeExportTest._AllMaterialConversions)


if __name__ == '__main__':
    unittest.main(verbosity=2)
