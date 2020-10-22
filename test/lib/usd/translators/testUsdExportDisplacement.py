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


class testUsdExportDisplacementShaders(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        mayaFile = os.path.join(inputPath, 'UsdExportDisplacementTest',
            'SimpleDisplacement.ma')
        cmds.file(mayaFile, force=True, open=True)

        # Export to USD.
        usdFilePath = os.path.abspath('SimpleDisplacement.usda')
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

    def testExportDisplacementShaders(self):
        '''
        Tests that exporting multiple Maya planes with varying displacement
        setups results in the expected USD data:
        '''

        textures = ["RGBA", "RGB", "MonoA", "Mono"]
        channels = ["r", "g", "b", "a", "a"]

        for suffix in range(1, 21):
            image = textures[int((suffix-1) / 5)]
            channel = channels[(suffix-1) % 5]

            mesh_prim = self._stage.GetPrimAtPath('/pPlane%i' % suffix)
            self.assertTrue(mesh_prim)

            # Validate the Material prim bound to the Mesh prim.
            self.assertTrue(mesh_prim.HasAPI(UsdShade.MaterialBindingAPI))
            mat_binding = UsdShade.MaterialBindingAPI(mesh_prim)
            material = mat_binding.ComputeBoundMaterial()[0]
            self.assertTrue(material)
            material_path = material.GetPath().pathString
            self.assertEqual(material_path,
                             '/pPlane%i/Materials/blinn%iSG' % (suffix,
                                                                suffix))

            self.assertTrue(material.ComputeSurfaceSource())
            self.assertTrue(material.ComputeDisplacementSource())

            # Validate the shader that is connected to the material.
            mat_out = material.GetOutput(UsdShade.Tokens.displacement)
            (connect_api, out_name, _) = mat_out.GetConnectedSource()
            self.assertEqual(out_name, UsdShade.Tokens.displacement)
            shader = UsdShade.Shader(connect_api)
            self.assertTrue(shader)
            shader_id = shader.GetIdAttr().Get()
            self.assertEqual(shader_id, 'UsdPreviewSurface')

            # Validate the connected input on the displacement shader.
            in_displacement = shader.GetInput('displacement')
            self.assertTrue(in_displacement)
            (connect_api, out_name, _) = in_displacement.GetConnectedSource()
            self.assertEqual(out_name, channel)
            shader = UsdShade.Shader(connect_api)
            self.assertTrue(shader)
            shader_id = shader.GetIdAttr().Get()
            self.assertEqual(shader_id, 'UsdUVTexture')
            filename = shader.GetInput("file")
            self.assertTrue(filename.Get().path.endswith("/%s.png" % image))


if __name__ == '__main__':
    unittest.main(verbosity=2)
