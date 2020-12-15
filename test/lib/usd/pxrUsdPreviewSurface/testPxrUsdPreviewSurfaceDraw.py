#!/pxrpythonsubst
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

from maya import cmds
from maya.app.general.mayaIsVP2Capable import mayaIsVP2Capable

import os
import sys
import unittest


class testPxrUsdPreviewSurfaceDraw(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        cls._testDir = os.path.join(os.path.abspath('.'),
                                    "PxrUsdPreviewSurfaceDrawTest")

        cls._cameraName = 'MainCamera'

    def setUp(self):
        cmds.file(new=True, force=True)

        # To control where the rendered images are written, we force Maya to
        # use the test directory as the workspace.
        cmds.workspace(self._testDir, o=True)

    def _WriteViewportImage(self, outputImageName, suffix):
        # Make sure the hardware renderer is available
        MAYA_RENDERER_NAME = 'mayaHardware2'

        # Make it the current renderer.
        cmds.setAttr('defaultRenderGlobals.currentRenderer', MAYA_RENDERER_NAME,
            type='string')
        # Set the image format to PNG.
        cmds.setAttr('defaultRenderGlobals.imageFormat', 32)
        # Set the render mode to shaded and textured.
        cmds.setAttr('hardwareRenderingGlobals.renderMode', 4)
        # Specify the output image prefix. The path to it is built from the
        # workspace directory.
        outPngName = os.path.join('%s_%s' % (outputImageName, suffix))
        cmds.setAttr('defaultRenderGlobals.imageFilePrefix',
            outPngName, type='string')
        # Apply the viewer's color transform to the rendered image, otherwise
        # it comes out too dark.
        cmds.setAttr("defaultColorMgtGlobals.outputTransformEnabled", 1)

        # Do the render.
        cmds.ogsRender(camera=self._cameraName, currentFrame=True, width=1920,
            height=1080)

    @unittest.skipIf(cmds.about(batch=True) or not mayaIsVP2Capable(),
                     "Requires a GUI and a valid VP2")
    def testDrawPxrUsdPreviewSurface(self):
        """
        Tests performing a Viewport 2.0 render of a collection of spheres
        bound to pxrUsdPreviewSurface materials. The spheres are arranged in
        rows with material assignments that ramp the attribute values through
        the typical ranges for those attributes.
        """
        self._testName = 'PxrUsdPreviewSurfaceDrawTest'

        mayaSceneFile = '%s.ma' % self._testName
        mayaSceneFullPath = os.path.abspath(mayaSceneFile)
        cmds.file(mayaSceneFullPath, open=True, force=True)

        self._WriteViewportImage(self._testName, 'value_ramps')


if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(testPxrUsdPreviewSurfaceDraw)

    results = unittest.TextTestRunner(stream=sys.__stderr__).run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1
    # Maya running interactively often absorbs all the output. Comment out the
    # following to prevent Maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)
