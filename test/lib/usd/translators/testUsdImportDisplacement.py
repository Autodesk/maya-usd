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

class testUsdImportDisplacement(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        input_path = fixturesUtils.setUpClass(__file__)

        cls.test_dir = os.path.join(input_path,
                                    "UsdImportDisplacementTest")

        cmds.workspace(cls.test_dir, o=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testDisplacement(self):
        """
        Tests that a UsdPreviewSurface file with displacement imports correctly.
        """

        usd_path = os.path.join(self.test_dir, "SimpleDisplacement.usda")
        options = ["shadingMode=[[useRegistry,UsdPreviewSurface]]",
                   "primPath=/",
                   "preferredMaterial=lambert"]
        cmds.file(usd_path, i=True, type="USD Import",
                  ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                  namespace="Test", pr=True, importTimeRange="combine",
                  options=";".join(options))

        # We should end up with a shading group that has both a lambert and a
        # displacement shader attached:
        sgs = cmds.listConnections("pPlane16Shape", type="shadingEngine")
        shaders = cmds.listConnections(sgs[0] + ".surfaceShader", d=False)
        node_type = cmds.nodeType(shaders[0])
        self.assertEqual(node_type, "lambert")

        shaders = cmds.listConnections(sgs[0] + ".displacementShader", d=False)
        node_type = cmds.nodeType(shaders[0])
        self.assertEqual(node_type, "displacementShader")

        # And the displacement shader has a texture on the displacement port:
        textures = cmds.listConnections(shaders[0] + ".displacement", d=False)
        node_type = cmds.nodeType(textures[0])
        self.assertEqual(node_type, "file")


if __name__ == '__main__':
    unittest.main(verbosity=2)
