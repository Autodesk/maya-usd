#!/usr/bin/env mayapy
#
# Copyright 2018 Pixar
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

from pxr import UsdMaya

from maya import cmds

import os
import sys
import unittest

import fixturesUtils
import imageUtils
import mayaUtils


class testRefAssemblyDrawRepresentations(imageUtils.ImageDiffingTestCase):

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        cls._testName = 'RefAssemblyDrawRepresentationsTest'
        inputPath = fixturesUtils.setUpClass(
            __file__, initializeStandalone=False, loadPlugin=False)

        cmds.loadPlugin('pxrUsd')

        cls._testDir = os.path.abspath('.')
        cls._inputDir = os.path.join(inputPath, cls._testName)

        cls._cameraName = 'MainCamera'

        # Make sure the relative-path to the USD file works by setting the current
        # directory to where that file is.
        os.chdir(cls._inputDir)

    def setUp(self):
        cmds.file(new=True, force=True)

        # To control where the rendered images are written, we force Maya to
        # use the test directory as the workspace.
        cmds.workspace(self._testDir, o=True)

    def _WriteViewportImage(self, outputImageName, suffix):
        # Make sure the hardware renderer is available
        MAYA_RENDERER_NAME = 'mayaHardware2'
        mayaRenderers = cmds.renderer(query=True, namesOfAvailableRenderers=True)
        self.assertIn(MAYA_RENDERER_NAME, mayaRenderers)

        # Make it the current renderer.
        cmds.setAttr('defaultRenderGlobals.currentRenderer', MAYA_RENDERER_NAME,
            type='string')
        # Set the image format to PNG.
        cmds.setAttr('defaultRenderGlobals.imageFormat', 32)
        # Set the render mode to shaded and textured.
        cmds.setAttr('hardwareRenderingGlobals.renderMode', 4)
        # Specify the output image prefix. The path to it is built from the
        # workspace directory.
        cmds.setAttr('defaultRenderGlobals.imageFilePrefix',
            '%s_%s' % (outputImageName, suffix),
            type='string')
        # Apply the viewer's color transform to the rendered image, otherwise
        # it comes out too dark.
        cmds.setAttr("defaultColorMgtGlobals.outputTransformEnabled", 1)

        # Do the render.
        #
        # We need to render twice, once in the input directory where the input
        # test file resides and once in the test output directory.
        #
        # The first render is needed because:
        #    1. The USD file used in the assembly lives in the input directory
        #    2. It uses a relative path
        #    3. It is only resolved when the Maya node gets computed
        #    4. The Maya node gets computed only when we try to render it
        #
        # So we need to do a first compute in the input directory so that the
        # input USD file is found.
        #
        # The second render is needed so that the output file is found in
        # the directory the test expects.

        # Make sure the relative-path input assembly USD file is found.
        cmds.ogsRender(camera="persp", currentFrame=True, width=960, height=540)

        # Make sure the image is written in the test output folder.
        os.chdir(self._testDir)
        cmds.ogsRender(camera=self._cameraName, currentFrame=True, width=960,
            height=540)

        imageName = '%s_%s.png' % (outputImageName, suffix)
        baselineImagePath = os.path.join(self._inputDir, 'baseline', imageName)
        outputImagePath = os.path.join('.', 'tmp', imageName)

        self.assertImagesClose(baselineImagePath, outputImagePath)

        # Make sure the relative-path to the USD file works by setting the current
        # directory to where that file is.
        os.chdir(self._inputDir)

    def testDrawCubeRepresentations(self):
        """
        Tests drawing USD proxy shapes while changing the assembly
        representation.
        """
        mayaSceneFile = '%s.ma' % self._testName
        mayaSceneFullPath = os.path.join(self._inputDir, mayaSceneFile)
        cmds.file(mayaSceneFullPath, open=True, force=True)

        UsdMaya.LoadReferenceAssemblies()

        for representation in ['Cards', 'Collapsed', 'Expanded', 'Full',
                'Playback']:
            
           # The cards rendering colors in older versions of Maya is lighter,
            suffix = ''
            if mayaUtils.mayaMajorVersion() <= 2024:
                if representation == "Cards":
                    suffix = '_v1'

            cmds.assembly("Cube_1", edit=True, active=representation)
             
            cmds.assembly('Cube_1', edit=True, active=representation)
            self._WriteViewportImage(self._testName, representation)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
