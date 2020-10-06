#!/pxrpythonsubst
#
# Copyright 2020 Pixar
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

import os
import unittest

import fixturesUtils


class testUsdExportRfMShaders(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        mayaFile = os.path.join(inputPath, 'UsdExportRfMShadersTest',
            'MarbleCube.ma')
        cmds.file(mayaFile, force=True, open=True)

        # Export to USD.
        usdFilePath = os.path.abspath('MarbleCube.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo='rendermanForMaya',
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

    def testExportRfMShaders(self):
        '''
        Tests that exporting a Maya mesh with a simple Maya shading setup
        results in the correct shading on the USD mesh.
        '''
        cubePrim = self._stage.GetPrimAtPath('/MarbleCube/Geom/Cube')
        self.assertTrue(cubePrim)

        # Validate the Material prim bound to the Mesh prim.
        self.assertTrue(cubePrim.HasAPI(UsdShade.MaterialBindingAPI))
        materialBindingAPI = UsdShade.MaterialBindingAPI(cubePrim)
        material = materialBindingAPI.ComputeBoundMaterial()[0]
        self.assertTrue(material)
        materialPath = material.GetPath().pathString
        self.assertEqual(materialPath, '/MarbleCube/Materials/MarbleCubeSG')

        # We expect four outputs on the material, the three built-in terminals
        # for the universal renderContext, and a fourth for the "surface"
        # terminal in the "ri" renderContext that the export should have
        # authored.
        materialOutputs = material.GetOutputs()
        self.assertEqual(len(materialOutputs), 4)

        # Validate the lambert surface shader that is connected to the material.
        materialOutput = material.GetOutput('ri:surface')
        (connectableAPI, outputName, outputType) = materialOutput.GetConnectedSource()
        self.assertEqual(outputName, UsdShade.Tokens.surface)
        shader = UsdShade.Shader(connectableAPI)
        self.assertTrue(shader)
        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaLambert')

        # Validate the connected input on the lambert surface shader.
        shaderInput = shader.GetInput('color')
        self.assertTrue(shaderInput)
        (connectableAPI, outputName, outputType) = shaderInput.GetConnectedSource()
        self.assertEqual(outputName, 'outColor')
        shader = UsdShade.Shader(connectableAPI)
        self.assertTrue(shader)
        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaMarble')

        # Validate the connected input on the marble shader.
        shaderInput = shader.GetInput('placementMatrix')
        self.assertTrue(shaderInput)
        (connectableAPI, outputName, outputType) = shaderInput.GetConnectedSource()
        self.assertEqual(outputName, 'worldInverseMatrix')
        shader = UsdShade.Shader(connectableAPI)
        self.assertTrue(shader)
        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaPlacement3d')

    def testShaderAttrsAuthoredSparsely(self):
        '''
        Tests that only the attributes authored in Maya are exported to USD.
        '''
        shaderPrimPath = '/MarbleCube/Materials/MarbleCubeSG/MarbleLambert'
        shaderPrim = self._stage.GetPrimAtPath(shaderPrimPath)
        self.assertTrue(shaderPrim)
        shader = UsdShade.Shader(shaderPrim)
        self.assertTrue(shader)
        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaLambert')
        shaderInputs = shader.GetInputs()
        self.assertEqual(len(shaderInputs), 1)

        # We actually expect two outputs on the "top-level" shader. The Maya
        # node this shader came from would have had an "outColor" output that
        # should have been translated, and then a "surface" output should have
        # been created when the shader was bound into the Material.
        shaderOutputs = shader.GetOutputs()
        self.assertEqual(len(shaderOutputs), 2)
        outputNames = {o.GetBaseName() for o in shaderOutputs}
        self.assertEqual(outputNames, set(['outColor', 'surface']))

        # Traverse the network down to the marble shader
        shaderInput = shader.GetInput('color')
        self.assertTrue(shaderInput)
        (connectableAPI, outputName, outputType) = shaderInput.GetConnectedSource()
        shader = UsdShade.Shader(connectableAPI)
        self.assertTrue(shader)

        inputPlacementMatrix = shader.GetInput('placementMatrix')
        (connectableAPI, outputName, outputType) = inputPlacementMatrix.GetConnectedSource()
        self.assertEqual(connectableAPI.GetPath().pathString,
            '/MarbleCube/Materials/MarbleCubeSG/MarbleCubePlace3dTexture')
        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaMarble')
        shaderOutputs = shader.GetOutputs()
        self.assertEqual(len(shaderOutputs), 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
