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


class testUsdExportImportUDIM(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        input_path = fixturesUtils.setUpClass(__file__)

        cls.test_dir = os.path.join(input_path,
                                    "UsdExportImportUDIMTest")

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportUDIM(self):
        '''
        Tests that exporting a UDIM texture works:
        '''

        maya_file = os.path.join(self.test_dir, 'UsdExportUDIMTest.ma')
        cmds.file(maya_file, force=True, open=True)

        # Export to USD.
        usd_file = os.path.abspath('UsdExportUDIMTest.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usd_file,
            shadingMode='useRegistry', convertMaterialsTo='UsdPreviewSurface',
            materialsScopeName='Materials')

        stage = Usd.Stage.Open(usd_file)
        self.assertTrue(stage)

        shader_prim = stage.GetPrimAtPath('/pPlane1/Materials/lambert2SG/file1')
        self.assertTrue(shader_prim)

        shader = UsdShade.Shader(shader_prim)
        shader_id = shader.GetIdAttr().Get()
        self.assertEqual(shader_id, 'UsdUVTexture')
        filename = shader.GetInput("file")
        self.assertTrue("<UDIM>" in filename.Get().path)

    def testImportUDIM(self):
        '''
        Tests that importing a UDIM texture works:
        '''
        usd_path = os.path.join(self.test_dir, "UsdImportUDIMTest.usda")
        options = ["shadingMode=[[useRegistry,UsdPreviewSurface]]",
                   "primPath=/"]
        cmds.file(usd_path, i=True, type="USD Import",
                  ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                  namespace="Test", pr=True, importTimeRange="combine",
                  options=";".join(options))

        self.assertEqual(cmds.getAttr("file1.uvTilingMode"), 3)
        # Path was resolved:
        self.assertTrue(os.path.isabs(cmds.getAttr("file1.fileTextureName")))


if __name__ == '__main__':
    unittest.main(verbosity=2)
