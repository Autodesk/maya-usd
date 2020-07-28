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

import os
import unittest

from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdImportPreviewSurface(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        input_path = fixturesUtils.setUpClass(__file__)

        cls.test_dir = os.path.join(input_path,
                                    "UsdImportPreviewSurface")

        cmds.workspace(cls.test_dir, o=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testImportReferenceWithRelativeFilePath(self):
        """
        Tests that a nested file node with a reference file path resolves
        correctly.
        """
        plugin_name = "mayaUsdPlugin"
        if not cmds.pluginInfo(plugin_name, query=True, loaded=True):
            cmds.loadPlugin(plugin_name)

        cmds.file(f=True, new=True)

        usd_path = os.path.join(self.test_dir, "world.usda")
        cmds.file(usd_path, i=True, type="USD Import",
                  ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                  namespace="Test", pr=True, importTimeRange="combine",
                  options=";shadingMode=useRegistry;primPath=/")

        # Check that all paths are resolved:
        filename = cmds.getAttr("nestedFile.fileTextureName")
        self.assertTrue(filename.endswith("green_A.png"))
        self.assertFalse(filename.startswith("textures"))

        filename = cmds.getAttr("upOneLevelFile.fileTextureName")
        self.assertTrue(filename.endswith("black_B.png"))
        self.assertFalse(filename.startswith(".."))

        filename = cmds.getAttr("upTwoLevelsFile.fileTextureName")
        self.assertTrue(filename.endswith("red_C.png"))
        self.assertFalse(filename.startswith(".."))

        filename = cmds.getAttr("nestedFile1.fileTextureName")
        self.assertTrue(filename.endswith("black_B.png"))
        self.assertFalse(filename.startswith("textures"))

        filename = cmds.getAttr("upOneLevelFile1.fileTextureName")
        self.assertTrue(filename.endswith("red_C.png"))
        self.assertFalse(filename.startswith(".."))

        filename = cmds.getAttr("upTwoLevelsFile1.fileTextureName")
        self.assertTrue(filename.endswith("green_A.png"))
        self.assertFalse(filename.startswith(".."))

        # Last one can not be resolved. Make sure it remained relative
        filename = cmds.getAttr("unresolvableFile.fileTextureName")
        self.assertTrue(filename.endswith("unresolvable.png"))
        self.assertTrue(filename.startswith(".."))


if __name__ == '__main__':
    unittest.main(verbosity=2)
