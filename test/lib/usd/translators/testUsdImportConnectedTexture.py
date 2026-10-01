#!/usr/bin/env mayapy
#
# Copyright 2026 Autodesk
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


class testUsdImportConnectedTexture(unittest.TestCase):
    """
    Tests that a UsdUVTexture whose inputs:file is not locally authored but
    instead connected to a Material interface input resolves the texture path
    correctly on import.  This exercises the connection-following logic in
    ResolveTextureAssetPath / handleShaderInput.
    """

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.readOnlySetUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testConnectedTextureFileResolved(self):
        """
        Importing a UsdUVTexture whose inputs:file is connected to a Material
        interface input must yield the asset path that was authored on that
        interface input, not an empty string.
        """
        cmds.file(f=True, new=True)

        usdPath = os.path.join(self.inputPath, "UsdImportConnectedTexture", "UsdImportConnectedTexture.usda")
        options = [
            "shadingMode=[[useRegistry,UsdPreviewSurface]]",
            "primPath=/",
            "preferredMaterial=none",
        ]
        cmds.file(usdPath, i=True, type="USD Import",
                  ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                  namespace="Test", pr=True, importTimeRange="combine",
                  options=";".join(options))

        # ConnectedFile: inputs:file connected to the Material interface input.
        # The texture path must be recovered by following that connection.
        connectedPath = cmds.getAttr("Test:ConnectedFile.fileTextureName")
        self.assertFalse(
            connectedPath == "",
            "fileTextureName must not be empty when inputs:file is connected "
            "to a Material interface input",
        )
        self.assertIn(
            "connected_texture.png",
            connectedPath.replace("\\", "/"),
        )

        # LocalFile: inputs:file is locally authored on the shader.
        # This is the baseline path, exercising the direct Get() in
        # handleShaderInput.
        localPath = cmds.getAttr("Test:LocalFile.fileTextureName")
        self.assertFalse(
            localPath == "",
            "fileTextureName must not be empty when inputs:file is locally authored",
        )
        self.assertIn(
            "local_texture.png",
            localPath.replace("\\", "/"),
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
