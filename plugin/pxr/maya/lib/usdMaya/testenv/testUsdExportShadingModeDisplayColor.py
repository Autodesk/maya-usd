#!/pxrpythonsubst
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

from pxr import Gf
from pxr import Usd
from pxr import UsdGeom
from pxr import UsdShade

import mayaUsd.lib as mayaUsdLib

from maya import cmds
from maya import standalone

import os
import unittest


class testUsdExportShadingModeDisplayColor(unittest.TestCase):

    RED_COLOR = 0.8 * Gf.Vec3f(1.0, 0.0, 0.0)

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        mayaFile = os.path.abspath('RedCube.ma')
        cmds.file(mayaFile, open=True, force=True)

        # Export to USD.
        usdFilePath = os.path.abspath('RedCube.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='displayColor', materialsScopeName='Materials')

        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._stage)

    def testExportDisplayColorShading(self):
        """
        Tests that exporting a Maya mesh with a simple Maya shading setup
        results in the correct shading on the USD mesh.
        """
        # Validate the displayColor on the mesh prim.
        cubePrim = self._stage.GetPrimAtPath('/RedCube/Geom/Cube')
        self.assertTrue(cubePrim)

        cubeMesh = UsdGeom.Mesh(cubePrim)
        self.assertTrue(cubeMesh)

        meshDisplayColors = cubeMesh.GetDisplayColorPrimvar().Get()
        self.assertEqual(len(meshDisplayColors), 1)
        self.assertTrue(Gf.IsClose(meshDisplayColors[0], 
            mayaUsdLib.ConvertMayaToLinear(self.RED_COLOR), 
            1e-6))

        # Validate the Material prim bound to the Mesh prim.
        materialBindingAPI = UsdShade.MaterialBindingAPI(cubePrim)
        material = materialBindingAPI.ComputeBoundMaterial()[0]
        self.assertTrue(material)
        materialPath = material.GetPath().pathString
        self.assertEqual(materialPath, '/RedCube/Materials/RedLambertSG')

        materialInputs = material.GetInputs()
        self.assertEqual(len(materialInputs), 3)

        materialInput = material.GetInput('displayColor')
        matDisplayColor = materialInput.Get()
        self.assertTrue(Gf.IsClose(matDisplayColor,
            mayaUsdLib.ConvertMayaToLinear(self.RED_COLOR), 
            1e-6))

        # Just verify that displayOpacity and transparency exist.
        materialInput = material.GetInput('displayOpacity')
        self.assertTrue(materialInput)

        materialInput = material.GetInput('transparency')
        self.assertTrue(materialInput)

        # Validate the surface shader that is connected to the material.
        materialOutputs = material.GetOutputs()
        self.assertEqual(len(materialOutputs), 4)
        print self._stage.ExportToString()
        materialOutput = material.GetOutput('ri:surface')
        (connectableAPI, outputName, outputType) = materialOutput.GetConnectedSource()
        self.assertEqual(outputName, 'out')
        shader = UsdShade.Shader(connectableAPI)
        self.assertTrue(shader)
        self.assertEqual(shader.GetPrim().GetName(), 'RedLambertSG_lambert')

        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrDiffuse')

        shaderInputs = shader.GetInputs()
        self.assertEqual(len(shaderInputs), 2)

        diffuseInput = shader.GetInput('diffuseColor')
        self.assertTrue(diffuseInput)
        (connectableAPI, outputName, outputType) = diffuseInput.GetConnectedSource()
        self.assertEqual(outputName, 'displayColor')
        self.assertTrue(connectableAPI)
        self.assertEqual(connectableAPI.GetPath().pathString, materialPath)

        transmissionInput = shader.GetInput('transmissionColor')
        self.assertTrue(transmissionInput)
        (connectableAPI, outputName, outputType) = transmissionInput.GetConnectedSource()
        self.assertEqual(outputName, 'transparency')
        self.assertTrue(connectableAPI)
        self.assertEqual(connectableAPI.GetPath().pathString, materialPath)


if __name__ == '__main__':
    unittest.main(verbosity=2)
