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


def connectUVNode(uv_node, file_node):
    for att_name in (".coverage", ".translateFrame", ".rotateFrame",
                    ".mirrorU", ".mirrorV", ".stagger", ".wrapU",
                    ".wrapV", ".repeatUV", ".offset", ".rotateUV",
                    ".noiseUV", ".vertexUvOne", ".vertexUvTwo",
                    ".vertexUvThree", ".vertexCameraOne"):
        cmds.connectAttr(uv_node + att_name, file_node + att_name, f=True)
    cmds.connectAttr(uv_node + ".outUV", file_node + ".uvCoord", f=True)
    cmds.connectAttr(uv_node + ".outUvFilterSize", file_node + ".uvFilterSize", f=True)


class testUsdExportMultiMaterial(unittest.TestCase):

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

    def testVarnameMerging(self):
        """
        Test that we produce a minimal number of UV readers and varname/varnameStr and that the
        connections are properly propagated across nodegraph boundaries.
        """
        cmds.file(f=True, new=True)

        # Use a namespace, for testing name sanitization...
        cmds.namespace(add="M")
        cmds.namespace(set="M")

        sphere_xform = cmds.polySphere()[0]

        material_node = cmds.shadingNode("usdPreviewSurface", asShader=True,
                                            name="ss01")
        material_sg = cmds.sets(renderable=True, noSurfaceShader=True,
                                empty=True, name="ss01SG")
        cmds.connectAttr(material_node+".outColor",
                            material_sg+".surfaceShader", force=True)
        cmds.sets(sphere_xform, e=True, forceElement=material_sg)

        # One file with UVs connected to diffuse:
        file_node = cmds.shadingNode("file", asTexture=True,
                                     isColorManaged=True)
        uv_node = cmds.shadingNode("place2dTexture", asUtility=True)
        cmds.setAttr(uv_node + ".offsetU", 0.125)
        cmds.setAttr(uv_node + ".offsetV", 0.5)
        connectUVNode(uv_node, file_node)
        cmds.connectAttr(file_node + ".outColor",
                         material_node + ".diffuseColor", f=True)

        # Another file, same UVs, connected to emissiveColor
        file_node = cmds.shadingNode("file", asTexture=True,
                                     isColorManaged=True)
        connectUVNode(uv_node, file_node)
        cmds.connectAttr(file_node + ".outColor",
                         material_node + ".emissiveColor", f=True)

        # Another file, no UVs, connected to metallic
        file_node = cmds.shadingNode("file", asTexture=True,
                                     isColorManaged=True)
        cmds.connectAttr(file_node + ".outColorR",
                         material_node + ".metallic", f=True)

        # Another file, no UVs, connected to roughness
        file_node = cmds.shadingNode("file", asTexture=True,
                                     isColorManaged=True)
        cmds.connectAttr(file_node + ".outColorR",
                         material_node + ".roughness", f=True)
        cmds.setAttr(file_node + ".offsetU", 0.25)
        cmds.setAttr(file_node + ".offsetV", 0.75)

        # Export to USD:
        usd_path = os.path.abspath('MinimalUVReader.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usd_path,
            shadingMode='useRegistry',
            convertMaterialsTo=['MaterialX', 'UsdPreviewSurface'])

        # We expect 2 primvar readers, and 2 st transforms:
        stage = Usd.Stage.Open(usd_path)
        mat_path = "/M_pSphere1/Looks/M_ss01SG"

        # Here are the expected connections in the produced USD file:
        connections = [
            # UsdPreviewSurface section

            # Source node, input, destination node:
            ("/UsdPreviewSurface/M_ss01", "diffuseColor", "/UsdPreviewSurface/M_file1"),
            ("/UsdPreviewSurface/M_file1", "st", "/UsdPreviewSurface/M_place2dTexture1_UsdTransform2d"),
            ("/UsdPreviewSurface/M_place2dTexture1_UsdTransform2d", "in", "/UsdPreviewSurface/M_place2dTexture1"),

            ("/UsdPreviewSurface/M_ss01", "emissiveColor", "/UsdPreviewSurface/M_file2"),
            ("/UsdPreviewSurface/M_file2", "st", "/UsdPreviewSurface/M_place2dTexture1_UsdTransform2d"), # re-used
            # Note that the transform name is derived from place2DTexture name.

            ("/UsdPreviewSurface/M_ss01", "metallic", "/UsdPreviewSurface/M_file3"),
            ("/UsdPreviewSurface/M_file3", "st", "/UsdPreviewSurface/shared_TexCoordReader"), # no UV in Maya.

            ("/UsdPreviewSurface/M_ss01", "roughness", "/UsdPreviewSurface/M_file4"),
            ("/UsdPreviewSurface/M_file4", "st", "/UsdPreviewSurface/M_file4_UsdTransform2d"), # xform on file node
            ("/UsdPreviewSurface/M_file4_UsdTransform2d", "in", "/UsdPreviewSurface/shared_TexCoordReader"),
            # Note that the transform name is derived from file node name.

            # MaterialX section

            # Source node, input, destination node:
            ("/MaterialX/MayaNG_MaterialX", "diffuseColor", "/MaterialX/MayaNG_MaterialX/MayaConvert_M_file1_MayafileTexture"),
            ("/MaterialX/MayaNG_MaterialX/MayaConvert_M_file1_MayafileTexture", "in", "/MaterialX/MayaNG_MaterialX/M_file1_MayafileTexture"),
            ("/MaterialX/MayaNG_MaterialX/M_file1_MayafileTexture", "inColor", "/MaterialX/MayaNG_MaterialX/M_file1"),
            ("/MaterialX/MayaNG_MaterialX/M_file1_MayafileTexture", "uvCoord", "/MaterialX/MayaNG_MaterialX/M_place2dTexture1"),
            ("/MaterialX/MayaNG_MaterialX/M_file1", "texcoord", "/MaterialX/MayaNG_MaterialX/M_place2dTexture1"),

            ("/MaterialX/MayaNG_MaterialX", "emissiveColor", "/MaterialX/MayaNG_MaterialX/MayaConvert_M_file2_MayafileTexture"),
            ("/MaterialX/MayaNG_MaterialX/MayaConvert_M_file2_MayafileTexture", "in", "/MaterialX/MayaNG_MaterialX/M_file2_MayafileTexture"),
            ("/MaterialX/MayaNG_MaterialX/M_file2_MayafileTexture", "inColor", "/MaterialX/MayaNG_MaterialX/M_file2"),
            ("/MaterialX/MayaNG_MaterialX/M_file2_MayafileTexture", "uvCoord", "/MaterialX/MayaNG_MaterialX/M_place2dTexture1"), # re-used
            ("/MaterialX/MayaNG_MaterialX/M_file2", "texcoord", "/MaterialX/MayaNG_MaterialX/M_place2dTexture1"), # re-used

            ("/MaterialX/MayaNG_MaterialX", "metallic", "/MaterialX/MayaNG_MaterialX/MayaSwizzle_M_file3_MayafileTexture_r"),
            ("/MaterialX/MayaNG_MaterialX/MayaSwizzle_M_file3_MayafileTexture_r", "in", "/MaterialX/MayaNG_MaterialX/M_file3_MayafileTexture"),
            ("/MaterialX/MayaNG_MaterialX/M_file3_MayafileTexture", "inColor", "/MaterialX/MayaNG_MaterialX/M_file3"),
            ("/MaterialX/MayaNG_MaterialX/M_file3_MayafileTexture", "uvCoord", "/MaterialX/MayaNG_MaterialX/shared_MayaGeomPropValue"),
            ("/MaterialX/MayaNG_MaterialX/M_file3", "texcoord", "/MaterialX/MayaNG_MaterialX/shared_MayaGeomPropValue"), # no UV in Maya.

            ("/MaterialX/MayaNG_MaterialX", "roughness", "/MaterialX/MayaNG_MaterialX/MayaSwizzle_M_file4_MayafileTexture_r"),
            ("/MaterialX/MayaNG_MaterialX/MayaSwizzle_M_file4_MayafileTexture_r", "in", "/MaterialX/MayaNG_MaterialX/M_file4_MayafileTexture"),
            ("/MaterialX/MayaNG_MaterialX/M_file4_MayafileTexture", "inColor", "/MaterialX/MayaNG_MaterialX/M_file4"),
            ("/MaterialX/MayaNG_MaterialX/M_file4_MayafileTexture", "uvCoord", "/MaterialX/MayaNG_MaterialX/shared_MayaGeomPropValue"), # re-used
            ("/MaterialX/MayaNG_MaterialX/M_file4", "texcoord", "/MaterialX/MayaNG_MaterialX/shared_MayaGeomPropValue"), # re-used

            # Making sure no NodeGraph boundaries were skipped downstream:
            ("", "surface", "/UsdPreviewSurface"),
            ("/UsdPreviewSurface", "surface", "/UsdPreviewSurface/M_ss01"),

            ("", "mtlx:surface", "/MaterialX"),
            ("/MaterialX", "surface", "/MaterialX/M_ss01"),

            # Making sure no NodeGraph boundaries were skipped upstream:
            ("/UsdPreviewSurface/M_place2dTexture1", "varname", "/UsdPreviewSurface"),
            ("/UsdPreviewSurface", "M:file1:varname", ""),

            ("/MaterialX/MayaNG_MaterialX/M_place2dTexture1_MayaGeomPropValue", "geomprop", "/MaterialX/MayaNG_MaterialX"),
            ("/MaterialX/MayaNG_MaterialX", "M:file1:varnameStr", "/MaterialX"),
            ("/MaterialX", "M:file1:varnameStr", ""),
        ]
        for src_name, input_name, dst_name in connections:
            src_prim = stage.GetPrimAtPath(mat_path + src_name)
            self.assertTrue(src_prim, mat_path + src_name + " does not exist")
            src_shade = UsdShade.ConnectableAPI(src_prim)
            self.assertTrue(src_shade)
            src_input = src_shade.GetInput(input_name)
            if not src_input:
                src_input = src_shade.GetOutput(input_name)
            self.assertTrue(src_input, input_name + " does not exist on " + mat_path + src_name)
            self.assertTrue(src_input.HasConnectedSource(), input_name + " does not have source")
            (connect_api, out_name, _) = src_input.GetConnectedSource()
            self.assertEqual(connect_api.GetPath(), mat_path + dst_name)

if __name__ == '__main__':
    unittest.main(verbosity=2)
