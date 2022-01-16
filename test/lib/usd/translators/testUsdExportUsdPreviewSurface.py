#!/usr/bin/env mayapy
#
# Copyright 2021 Apple Inc. All rights reserved.
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

import os
import unittest

import fixturesUtils
from maya import cmds
from maya import standalone

from pxr import Gf
from pxr import Sdf
from pxr import Usd
from pxr import UsdShade
from pxr import UsdUtils


class testUsdExportUsdPreviewSurface(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.input_dir = fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath(".")

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _GetUsdMaterial(self, stage, materialName):
        modelPrimPath = Sdf.Path.absoluteRootPath.AppendChild(
            "UsdPreviewSurfaceExportTest"
        )
        materialsRootPrimPath = modelPrimPath.AppendChild(
            UsdUtils.GetMaterialsScopeName()
        )
        materialPrimPath = materialsRootPrimPath.AppendChild(materialName)
        materialPrim = stage.GetPrimAtPath(materialPrimPath)
        self.assertTrue(materialPrim)

        materialSchema = UsdShade.Material(materialPrim)
        self.assertTrue(materialSchema)

        return materialSchema

    def _GetSourceShader(self, inputOrOutput):
        (connectableAPI, _, _) = inputOrOutput.GetConnectedSource()
        self.assertTrue(connectableAPI.GetPrim().IsA(UsdShade.Shader))
        shaderPrim = connectableAPI.GetPrim()
        self.assertTrue(shaderPrim)

        shader = UsdShade.Shader(shaderPrim)
        self.assertTrue(shader)

        return shader

    def _ValidateUsdShader(self, shader, expectedInputTuples, expectedOutputs):
        for expectedInputTuple in expectedInputTuples:
            (inputName, expectedValue) = expectedInputTuple

            shaderInput = shader.GetInput(inputName)
            self.assertTrue(shaderInput)

            if expectedValue is None:
                self.assertFalse(shaderInput.GetAttr().HasAuthoredValueOpinion())
                continue

            # Validate the input value
            value = shaderInput.Get()
            if isinstance(value, float) or isinstance(value, Gf.Vec3f):
                self.assertTrue(Gf.IsClose(value, expectedValue, 1e-6))
            else:
                self.assertEqual(value, expectedValue)

        outputs = {
            output.GetBaseName(): output.GetTypeName() for output in shader.GetOutputs()
        }

        self.assertEqual(outputs, expectedOutputs)

    def _ValidateUsdShaderOuputs(self, shader, expectedOutputs):

        # Validate shader only has one output
        self.assertEqual(len(shader.GetOutputs()), 1)

        # Validate output's value
        shaderOuput = shader.GetOutputs()[0]
        self.assertEqual(shaderOuput.GetBaseName(), expectedOutputs)

    def generateStandaloneTestScene(self, attrTuples):
        """
        Generate test scene containing a usdPreviewSurface with attribute values but no
        connections authored exports correctly.
        """
        maya_file = os.path.join(
            self.temp_dir, "UsdExportStandaloneUsdPreviewSurfaceTest.ma"
        )
        cmds.file(force=True, new=True)
        mesh = "StandaloneMaterialSphere"
        cmds.polySphere(name=mesh, subdivisionsX=20, subdivisionsY=20, radius=1)
        cmds.group(mesh, name="Geom")
        cmds.group("Geom", name="UsdPreviewSurfaceExportTest")
        shading_node = "usdPreviewSurface_Standalone"
        cmds.shadingNode("usdPreviewSurface", name=shading_node, asShader=True)

        for attr in attrTuples:
            if isinstance(attr[1], Gf.Vec3f):
                cmds.setAttr(
                    "%s.%s" % (shading_node, attr[0]),
                    attr[1][0],
                    attr[1][1],
                    attr[1][2],
                )
            else:
                cmds.setAttr("%s.%s" % (shading_node, attr[0]), attr[1])

        shading_engine = "%sSG" % shading_node
        cmds.sets(
            renderable=True, noSurfaceShader=True, empty=True, name=shading_engine
        )
        cmds.connectAttr(
            "%s.outColor" % shading_node,
            "%s.surfaceShader" % shading_engine,
            force=True,
        )
        cmds.sets(mesh, edit=True, forceElement=shading_engine)

        cmds.file(rename=maya_file)
        cmds.file(save=True, type="mayaAscii")
        self.assertTrue(os.path.exists(maya_file))

        return maya_file

    def generateConnectedTestScene(self, shadingNodeAttributes, ignoreColorSpaceFileRules=False):
        """
        Generate test scene containing a UsdPreviewSurface with bindings to other shading nodes
        exports correctly.
        :type shadingNodeAttributes: List[Tuple[str, Any]]
        :type ignoreColorSpaceFileRules: bool
        """
        maya_file = os.path.join(
            self.temp_dir, "UsdExportConnectedUsdPreviewSurfaceTest.ma"
        )
        cmds.file(force=True, new=True)
        mesh = "ConnectedMaterialSphere"
        cmds.polySphere(name=mesh, subdivisionsX=20, subdivisionsY=20, radius=1)
        cmds.group(mesh, name="Geom")
        cmds.group("Geom", name="UsdPreviewSurfaceExportTest")
        shading_node = "usdPreviewSurface_Connected"
        cmds.shadingNode("usdPreviewSurface", name=shading_node, asShader=True)

        for attr in shadingNodeAttributes:
            if isinstance(attr[1], Gf.Vec3f):
                cmds.setAttr(
                    "%s.%s" % (shading_node, attr[0]),
                    attr[1][0],
                    attr[1][1],
                    attr[1][2],
                )
            else:
                cmds.setAttr("%s.%s" % (shading_node, attr[0]), attr[1])

        texture_dir = os.path.join(self.input_dir, "UsdExportUsdPreviewSurfaceTest")
        cmds.defaultNavigation(
            createNew=True, destination="%s.diffuseColor" % shading_node
        )
        file_node = cmds.shadingNode(
            "file", asTexture=True, name="Brazilian_Rosewood_Texture"
        )
        cmds.setAttr(file_node + ".ignoreColorSpaceFileRules", ignoreColorSpaceFileRules)

        cmds.setAttr(
            file_node + ".fileTextureName",
            os.path.join(texture_dir, "Brazilian_rosewood_pxr128.png"),
            type="string",
        )
        cmds.connectAttr(
            "%s.outColor" % file_node, "%s.diffuseColor" % shading_node, force=True
        )

        # This file node should have stayed "sRGB":
        if not ignoreColorSpaceFileRules:
            self.assertEqual(cmds.getAttr(file_node + ".colorSpace"), "sRGB")

        cmds.defaultNavigation(
            createNew=True, destination="%s.roughness" % shading_node
        )
        file_node = cmds.shadingNode(
            "file", asTexture=True, name="Brazilian_Rosewood_Bump_Texture"
        )
        cmds.setAttr(file_node + ".ignoreColorSpaceFileRules", ignoreColorSpaceFileRules)

        cmds.setAttr(
            file_node + ".fileTextureName",
            os.path.join(texture_dir, "Brazilian_rosewood_pxr128_bmp.png"),
            type="string",
        )
        cmds.connectAttr(
            "%s.outColorR" % file_node, "%s.roughness" % shading_node, force=True
        )

        # The monochrome file node should have been set to "Raw" automatically:
        if not ignoreColorSpaceFileRules:
            self.assertEqual(cmds.getAttr(file_node + ".colorSpace"), "Raw")

        cmds.defaultNavigation(
            createNew=True, destination="%s.clearcoatRoughness" % shading_node
        )
        cmds.connectAttr(
            "%s.outColorR" % file_node,
            "%s.clearcoatRoughness" % shading_node,
            force=True,
        )

        cmds.defaultNavigation(createNew=True, destination="%s.normal" % shading_node)
        file_node = cmds.shadingNode(
            "file", asTexture=True, name="Brazilian_Rosewood_Normal_Texture"
        )
        cmds.setAttr(
            file_node + ".fileTextureName",
            os.path.join(texture_dir, "Brazilian_rosewood_pxr128_n.png"),
            type="string",
        )
        cmds.connectAttr(
            "%s.outColor" % file_node, "%s.normal" % shading_node, force=True
        )

        # The file node should have been set to "NormalMap" automatically:
        self.assertEqual(cmds.getAttr(file_node + ".colorSpace"), "Raw")
        self.assertEqual(cmds.getAttr(file_node + ".colorGainR"), 2)
        self.assertEqual(cmds.getAttr(file_node + ".colorGainG"), 2)
        self.assertEqual(cmds.getAttr(file_node + ".colorGainB"), 2)
        self.assertEqual(cmds.getAttr(file_node + ".colorOffsetR"), -1)
        self.assertEqual(cmds.getAttr(file_node + ".colorOffsetG"), -1)
        self.assertEqual(cmds.getAttr(file_node + ".colorOffsetB"), -1)
        self.assertEqual(cmds.getAttr(file_node + ".alphaGain"), 1)
        self.assertEqual(cmds.getAttr(file_node + ".alphaOffset"), 0)

        shading_engine = "%sSG" % shading_node
        cmds.sets(
            renderable=True, noSurfaceShader=True, empty=True, name=shading_engine
        )
        cmds.connectAttr(
            "%s.outColor" % shading_node,
            "%s.surfaceShader" % shading_engine,
            force=True,
        )
        cmds.sets(mesh, edit=True, forceElement=shading_engine)

        cmds.file(rename=maya_file)
        cmds.file(save=True, type="mayaAscii")
        self.assertTrue(os.path.exists(maya_file))

        return maya_file

    def testExportStandaloneUsdPreviewSurface(self):
        """
        Tests that a usdPreviewSurface with attribute values but no
        connections authored exports correctly.
        """
        expectedInputTuples = [
            ("clearcoat", 0.1),
            ("clearcoatRoughness", 0.2),
            ("diffuseColor", Gf.Vec3f(0.3, 0.4, 0.5)),
            ("displacement", 0.6),
            ("emissiveColor", Gf.Vec3f(0.07, 0.08, 0.09)),
            ("ior", 1.1),
            ("metallic", 0.11),
            ("normal", Gf.Vec3f(0.12, 0.13, 0.14)),
            ("occlusion", 0.9),
            ("opacity", 0.8),
            ("roughness", 0.7),
            ("specularColor", Gf.Vec3f(0.3, 0.2, 0.1)),
            ("useSpecularWorkflow", 1),
        ]
        maya_file = self.generateStandaloneTestScene(expectedInputTuples)
        cmds.file(maya_file, force=True, open=True)
        usd_file_path = os.path.join(self.temp_dir, "UsdPreviewSurfaceExportTest.usda")
        cmds.mayaUSDExport(
            mergeTransformAndShape=True, file=usd_file_path, shadingMode="useRegistry"
        )

        stage = Usd.Stage.Open(usd_file_path)

        standaloneMaterial = self._GetUsdMaterial(
            stage, "usdPreviewSurface_StandaloneSG"
        )

        surfaceOutput = standaloneMaterial.GetOutput(UsdShade.Tokens.surface)
        previewSurfaceShader = self._GetSourceShader(surfaceOutput)

        expectedShaderPrimPath = standaloneMaterial.GetPath().AppendChild(
            "usdPreviewSurface_Standalone"
        )

        self.assertEqual(previewSurfaceShader.GetPath(), expectedShaderPrimPath)

        self.assertEqual(previewSurfaceShader.GetShaderId(), "UsdPreviewSurface")

        expectedOutputs = {
            "surface": Sdf.ValueTypeNames.Token,
            "displacement": Sdf.ValueTypeNames.Token,
        }

        self._ValidateUsdShader(
            previewSurfaceShader, expectedInputTuples, expectedOutputs
        )

        # There should not be any additional inputs.
        self.assertEqual(
            len(previewSurfaceShader.GetInputs()), len(expectedInputTuples)
        )

    def testExportConnectedUsdPreviewSurface(self):
        """
        Tests that a UsdPreviewSurface with bindings to other shading nodes
        exports correctly.
        """
        expectedInputTuples = [
            ("clearcoat", 0.1),
            ("specularColor", Gf.Vec3f(0.2, 0.2, 0.2)),
            ("useSpecularWorkflow", 1),
        ]
        maya_file = self.generateConnectedTestScene(expectedInputTuples, ignoreColorSpaceFileRules=False)
        cmds.file(maya_file, force=True, open=True)
        usd_file_path = os.path.join(self.temp_dir, "UsdPreviewSurfaceExportTest.usda")
        cmds.mayaUSDExport(
            mergeTransformAndShape=True, file=usd_file_path, shadingMode="useRegistry"
        )

        stage = Usd.Stage.Open(usd_file_path)

        connectedMaterial = self._GetUsdMaterial(stage, "usdPreviewSurface_ConnectedSG")

        surfaceOutput = connectedMaterial.GetOutput(UsdShade.Tokens.surface)
        previewSurfaceShader = self._GetSourceShader(surfaceOutput)

        expectedShaderPrimPath = connectedMaterial.GetPath().AppendChild(
            "usdPreviewSurface_Connected"
        )

        self.assertEqual(previewSurfaceShader.GetPath(), expectedShaderPrimPath)

        self.assertEqual(previewSurfaceShader.GetShaderId(), "UsdPreviewSurface")

        expectedOutputs = {
            "surface": Sdf.ValueTypeNames.Token,
            "displacement": Sdf.ValueTypeNames.Token,
        }

        self._ValidateUsdShader(
            previewSurfaceShader, expectedInputTuples, expectedOutputs
        )

        # There should be four more connected inputs in addition to the inputs
        # with authored values.
        self.assertEqual(
            len(previewSurfaceShader.GetInputs()), len(expectedInputTuples) + 4
        )

        # Validate the UsdUvTexture prim connected to the UsdPreviewSurface's
        # diffuseColor input.
        diffuseColorInput = previewSurfaceShader.GetInput("diffuseColor")
        difTexShader = self._GetSourceShader(diffuseColorInput)
        self._ValidateUsdShaderOuputs(difTexShader, "rgb")

        expectedShaderPrimPath = connectedMaterial.GetPath().AppendChild(
            "Brazilian_Rosewood_Texture"
        )

        self.assertEqual(difTexShader.GetPath(), expectedShaderPrimPath)
        self.assertEqual(difTexShader.GetShaderId(), "UsdUVTexture")

        # Validate the UsdUvTexture prim connected to the UsdPreviewSurface's
        # clearcoatRoughness and roughness inputs. They should both be fed by
        # the same shader prim.
        clearcoatRougnessInput = previewSurfaceShader.GetInput("clearcoatRoughness")
        bmpTexShader = self._GetSourceShader(clearcoatRougnessInput)
        rougnessInput = previewSurfaceShader.GetInput("roughness")
        roughnessShader = self._GetSourceShader(rougnessInput)

        self._ValidateUsdShaderOuputs(bmpTexShader, "r")
        self._ValidateUsdShaderOuputs(roughnessShader, "r")

        self.assertEqual(bmpTexShader.GetPrim(), roughnessShader.GetPrim())

        expectedShaderPrimPath = connectedMaterial.GetPath().AppendChild(
            "Brazilian_Rosewood_Bump_Texture"
        )

        self.assertEqual(bmpTexShader.GetPath(), expectedShaderPrimPath)
        self.assertEqual(bmpTexShader.GetShaderId(), "UsdUVTexture")

        # Validate the UsdUvTexture prim connected to the UsdPreviewSurface's
        # normal input.
        normalColorInput = previewSurfaceShader.GetInput("normal")
        normalTexShader = self._GetSourceShader(normalColorInput)

        self._ValidateUsdShaderOuputs(normalTexShader, "rgb")

        expectedShaderPrimPath = connectedMaterial.GetPath().AppendChild(
            "Brazilian_Rosewood_Normal_Texture"
        )

        self.assertEqual(normalTexShader.GetPath(), expectedShaderPrimPath)
        self.assertEqual(normalTexShader.GetShaderId(), "UsdUVTexture")

    def testIgnoreColorSpaceFileRules(self):
        """Test that color spaces don't get set when ignoreColorSpaceFileRules is set"""
        self.generateConnectedTestScene([], ignoreColorSpaceFileRules=True)


if __name__ == "__main__":
    unittest.main(verbosity=2)
