#!/usr/bin/env mayapy
#
# Copyright 2023 Autodesk
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

from maya import cmds
from maya import standalone

import os
import re
import tempfile
import unittest

import fixturesUtils


class testUsdImportRelativeTextures(unittest.TestCase):
    """
    Test importing with different modes for textures file paths.
    """

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.readOnlySetUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def runImportRelativeTexturesTest(self, relativeMode):
        """
        Tests that the textures file paths are what is expected.
        """

        # Ensure the project folder is set in Maya.
        projectFolder = os.path.join(self.inputPath, 'UsdImportRelativeTextures', 'rel_project')
        cmds.workspace(projectFolder, openWorkspace=True)

        usdFile = os.path.join(self.inputPath, "UsdImportRelativeTextures", "UsdImportRelativeTextures.usda")
        options = ["shadingMode=[[useRegistry,MaterialX]]",
                   "primPath=/",
                   "importRelativeTextures=%s" % relativeMode]
        cmds.file(usdFile, i=True, type="USD Import",
                  ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                  namespace="Test", pr=True, importTimeRange="combine",
                  options=";".join(options))
        
        tmpDir = tempfile.mkdtemp(prefix='ImportRelativeTexturesTest')
        mayaFile = os.path.join(tmpDir, 'ImportRelativeTexturesScene.ma')
        try:
            cmds.file(rename=mayaFile)
            cmds.file(save=True, force=True, type="mayaAscii")
            cmds.file(new=True, force=True)

            # Check texture file names
            expected = {
                "Test:file1": "Brazilian_rosewood_pxr128.png",
                "Test:file2": "RGB.png",
                "Test:file3": "normalSpiral.png",
            }

            # The getAttr command returns resolved file names.
            # The only way to validate that the names are relative to the project
            # is read the saved Maya ASCII file and find the file nodes and attribute
            # by hand.

            relProjectMarker = 'rel_project//..'

            fileRE = re.compile(r'''.*createNode file -n "([^"]+)";.*''')
            attrRE = re.compile(r'''.*setAttr ".ftn" -type "string" "([^"]+)";.*''')
            expectedFile = None

            with open(mayaFile, 'r') as f:
                for line in f:
                    m = fileRE.match(line)
                    if m:
                        fileNode = m.group(1)
                        if fileNode in expected:
                            expectedFile = expected[fileNode]
                        continue

                    m = attrRE.match(line)
                    if not m:
                        continue
                    if not expectedFile:
                        continue
                    textureFile = m.group(1)
                    self.assertIn(expectedFile, textureFile)
                    if relativeMode == 'absolute':
                        self.assertNotIn(relProjectMarker, textureFile)
                    else:
                        self.assertIn(relProjectMarker, textureFile)
        finally:
            if os.path.exists(mayaFile):
                os.remove(mayaFile)
            if os.path.exists(tmpDir):
                os.rmdir(tmpDir)


    def testImportRelativeTextures(self):
        """
        Tests that the textures file paths are relative.
        """
        self.runImportRelativeTexturesTest('relative')

    def testImportAbsoluteTextures(self):
        """
        Tests that the textures file paths are absolute.
        """
        self.runImportRelativeTexturesTest('absolute')

    def testImportAutomaticTextures(self):
        """
        Tests that the textures file paths mode is automatically detected.
        """
        self.runImportRelativeTexturesTest('automatic')

if __name__ == '__main__':
    unittest.main(verbosity=2)
