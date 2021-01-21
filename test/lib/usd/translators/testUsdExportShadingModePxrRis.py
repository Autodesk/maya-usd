#!/usr/bin/env mayapy
#
# Copyright 2017 Pixar
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

from pxr import Gf
from pxr import Usd
from pxr import UsdShade

from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdExportShadingModePxrRis(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        mayaFile = os.path.join(inputPath, "UsdExportShadingModePxrRis", "MarbleCube.ma")
        cmds.file(mayaFile, force=True, open=True)

        # Export to USD.
        usdFilePath = os.path.abspath('MarbleCube.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='pxrRis', materialsScopeName='Materials')

        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._stage)

    def testExportPxrRisShading(self):
        """
        Tests that exporting a Maya mesh with a simple Maya shading setup
        results in the correct shading on the USD mesh.
        """
        cubePrim = self._stage.GetPrimAtPath('/MarbleCube/Geom/Cube')
        self.assertTrue(cubePrim)

        # Validate the Material prim bound to the Mesh prim.
        materialBindingAPI = UsdShade.MaterialBindingAPI(cubePrim)
        material = materialBindingAPI.ComputeBoundMaterial()[0]
        self.assertTrue(material)
        materialPath = material.GetPath().pathString
        self.assertEqual(materialPath, '/MarbleCube/Materials/MarbleCubeSG')

        # Validate the surface shader that is connected to the material.
        materialOutputs = material.GetOutputs()
        self.assertEqual(len(materialOutputs), 4)
        print(self._stage.ExportToString())
        materialOutput = material.GetOutput('ri:surface')
        (connectableAPI, outputName, outputType) = materialOutput.GetConnectedSource()
        self.assertEqual(outputName, 'out')
        shader = UsdShade.Shader(connectableAPI)
        self.assertTrue(shader)

        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaMarble')

        # Validate the connected input on the surface shader.
        shaderInput = shader.GetInput('placementMatrix')
        self.assertTrue(shaderInput)

        (connectableAPI, outputName, outputType) = shaderInput.GetConnectedSource()
        self.assertEqual(outputName, 'worldInverseMatrix')
        shader = UsdShade.Shader(connectableAPI)
        self.assertTrue(shader)

        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaPlacement3d')

    def testShaderAttrsAuthoredSparsely(self):
        """
        Tests that only the attributes authored in Maya are exported to USD.
        """
        shaderPrimPath = '/MarbleCube/Materials/MarbleCubeSG/MarbleShader'
        shaderPrim = self._stage.GetPrimAtPath(shaderPrimPath)
        self.assertTrue(shaderPrim)

        shader = UsdShade.Shader(shaderPrim)
        self.assertTrue(shader)

        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaMarble')

        shaderInputs = shader.GetInputs()
        self.assertEqual(len(shaderInputs), 1)

        inputPlacementMatrix = shader.GetInput('placementMatrix')
        (connectableAPI, outputName, outputType) = inputPlacementMatrix.GetConnectedSource()
        self.assertEqual(connectableAPI.GetPath().pathString,
            '/MarbleCube/Materials/MarbleCubeSG/MarbleCubePlace3dTexture')

        shaderOutputs = shader.GetOutputs()
        self.assertEqual(len(shaderOutputs), 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
