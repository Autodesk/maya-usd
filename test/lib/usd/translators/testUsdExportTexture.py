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
import testUtils

PROJ_ENV_VAR_NAME = "_MAYA_USD_EXPORT_TEXTURE_TEST_PATH"

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
        try:
            # Cleanup the output folder added for the test. We don't want
            # to leave extra files around in the test source directory.
            outputFolder = os.path.dirname(self.getUsdFilePath())
            import shutil
            shutil.rmtree(outputFolder)
        except Exception:
            # Don't let cleanup errors fail the test.
            pass

    def runTextureTest(self, relativeMode, withEnvVar):
        projectFolder = os.path.join(self.inputPath, 'UsdExportTextureTest', 'rel_project')
        mayaScenePath = os.path.join(projectFolder, 'scenes', 'TextureTest.ma')
        cmds.file(mayaScenePath, force=True, open=True)

        # Ensure the project folder is set in Maya.
        cmds.workspace(projectFolder, openWorkspace=True)

        # Prepare texture path to be part of the project folder.
        # build the absolute texturePath, with an env var if requested
        textureRoot = ('$' + PROJ_ENV_VAR_NAME) if withEnvVar else projectFolder
        texturePath = os.path.join(textureRoot, 'sourceimages', 'grid.png')
        cmds.setAttr('file1.ftn', texturePath, edit=True, type='string')

        # Export to USD, ensuring envvar is set if it is used by texture  path
        usdFilePath = self.getUsdFilePath()

        with testUtils.TemporaryEnvironmentVariable(PROJ_ENV_VAR_NAME, projectFolder):
            cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath, 
                convertMaterialsTo=['UsdPreviewSurface'],
                exportRelativeTextures=relativeMode, legacyMaterialScope=False,
                defaultPrim='None')

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

        self.assertTrue(os.path.exists(exportedTexturePath.resolvedPath),
                        'The exported texture %s is not resolved by USD' % exportedTexturePath)

    def testExportRelativeTexture(self):
        '''
        Test that texture can be exported with relative paths for textures.
        '''
        self.runTextureTest('relative', withEnvVar=False)

    def testExportAbsoluteTexture(self):
        '''
        Test that texture can be exported with absolute paths for textures.
        '''
        self.runTextureTest('absolute', withEnvVar=False)

    def testExportAutomaticTexture(self):
        '''
        Test that texture can be exported with automatic reltive or absolute
        paths for textures.
        '''
        self.runTextureTest('automatic', withEnvVar=False)

    def testExportRelativeTextureWithEnvVar(self):
        '''
        Test that texture path with an environment variable expansion can be
        exported with relative paths for textures.
        '''
        self.runTextureTest('relative', withEnvVar=True)

    def testExportAbsoluteTextureWithEnvVar(self):
        '''
        Test that texture path with an environment variable expansion can be
        exported with absolute paths for textures.
        '''
        self.runTextureTest('absolute', withEnvVar=True)

    def testExportAutomaticTextureWithEnvVar(self):
        '''
        Test that texture path with an environment variable expansion can be
        exported with automatic relative or absolute paths for textures.
        '''
        self.runTextureTest('automatic', withEnvVar=True)


if __name__ == '__main__':
    unittest.main(verbosity=2)
