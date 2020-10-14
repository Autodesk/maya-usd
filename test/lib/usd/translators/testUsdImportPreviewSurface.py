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
        cmds.file(f=True, new=True)

        usd_path = os.path.join(self.test_dir, "world.usda")
        options = ["shadingMode=[[useRegistry,UsdPreviewSurface]]",
                   "primPath=/",
                   "preferredMaterial=none"]
        cmds.file(usd_path, i=True, type="USD Import",
                  ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                  namespace="Test", pr=True, importTimeRange="combine",
                  options=";".join(options))

        # Check that all paths are absolute:
        green_a = "props/billboards/textures/green_A.png"
        black_b = "props/textures/black_B.png"
        red_c = "textures/red_C.png"
        # Except this unresolvable path, left as-is:
        unresolvable = "../textures/unresolvable.png"

        rebased = (
            ("nestedFile", os.path.join(self.test_dir, green_a)),
            ("upOneLevelFile", os.path.join(self.test_dir, black_b)),
            ("upTwoLevelsFile", os.path.join(self.test_dir, red_c)),
            ("nestedFile1", os.path.join(self.test_dir, black_b)),
            ("upOneLevelFile1", os.path.join(self.test_dir, red_c)),
            ("upTwoLevelsFile1", os.path.join(self.test_dir, green_a)),
            ("unresolvableFile", unresolvable),
        )
        for node_name, rebased_name in rebased:
            filename = cmds.getAttr("%s.fileTextureName" % node_name)
            self.assertEqual(filename.lower().replace("\\", "/"),
                             rebased_name.lower().replace("\\", "/"))

        # We expect usdPreviewSurface shaders:
        self.assertEqual(len(cmds.ls(typ="usdPreviewSurface")), 8)

        # Re-import, but with lamberts:
        lambert_before = len(cmds.ls(typ="lambert"))
        options = options[:-1]
        options.append("preferredMaterial=lambert")
        cmds.file(usd_path, i=True, type="USD Import",
                  ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                  namespace="Test", pr=True, importTimeRange="combine",
                  options=";".join(options))

        self.assertEqual(len(cmds.ls(typ="lambert")), lambert_before + 8)

if __name__ == '__main__':
    unittest.main(verbosity=2)
