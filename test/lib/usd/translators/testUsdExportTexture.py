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

from pxr import Usd
from pxr import UsdShade
from pxr import Sdr

from maya import cmds
from maya import standalone

import os
import unittest

import fixturesUtils


class testUsdExportTexture(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def getUsdFilePath(self):
        # Note: the output file needs to be in the test folder so that
        #       it is possible to create relative paths, since we are not
        #       guaranteed that the build folder is on th esame drive as
        #       the source folder. For example, it is not in preflight testing.
        outputFolder = os.path.join(self.inputPath, 'UsdExportTextureTest', 'output')
        return os.path.join(outputFolder, 'TextureTest.usda')

    def tearDown(self):
        usdFilePath = self.getUsdFilePath()
        try:
            if os.path.exists(usdFilePath):
                os.remove(usdFilePath)
        except Exception:
            # Don't let cleanup errors fail the test.
            # We're removing the output file only to be nice, it is not mandatory.
            pass

    def runTextureTest(self, relativeMode):
        projectFolder = os.path.join(self.inputPath, 'UsdExportTextureTest', 'rel_project')
        mayaScenePath = os.path.join(projectFolder, 'scenes', 'TextureTest.ma')
        cmds.file(mayaScenePath, force=True, open=True)

        # Ensure the project folder is set in Maya.
        cmds.workspace(projectFolder, openWorkspace=True)

        # Prepare texture path to be part of the project folder.
        texturePath = os.path.join(projectFolder, 'sourceimages', 'grid.png')
        cmds.setAttr('file1.ftn', texturePath, edit=True, type='string')

        # Export to USD.
        usdFilePath = self.getUsdFilePath()
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath, exportRelativeTextures=relativeMode)

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage, usdFilePath)

        shaderWithTexturePrimPath = '/Looks/initialShadingGroup/file1'
        shaderWithTexturePrim = stage.GetPrimAtPath(shaderWithTexturePrimPath)
        self.assertTrue(shaderWithTexturePrim, shaderWithTexturePrimPath)

        fileAttrName = 'inputs:file'
        fileAttr = shaderWithTexturePrim.GetAttribute(fileAttrName)
        self.assertTrue(fileAttr, fileAttrName)
        exportedTexturePath = fileAttr.Get()
        self.assertTrue(exportedTexturePath, fileAttrName)
        shouldBeAbsolute = bool(relativeMode=='absolute')
        self.assertEqual(os.path.isabs(exportedTexturePath.path), shouldBeAbsolute,
                         'The exported texture %s does not have the right relative mode' % exportedTexturePath)

    def testExportRelativeTexture(self):
        '''
        Test that texture can be exported with relative paths for textures.
        '''
        self.runTextureTest('relative')

    def testExportAbsoluteTexture(self):
        '''
        Test that texture can be exported with absolute paths for textures.
        '''
        self.runTextureTest('absolute')

    def testExportAutomaticTexture(self):
        '''
        Test that texture can be exported with automatic reltive or absolute
        paths for textures.
        '''
        self.runTextureTest('automatic')


if __name__ == '__main__':
    unittest.main(verbosity=2)
