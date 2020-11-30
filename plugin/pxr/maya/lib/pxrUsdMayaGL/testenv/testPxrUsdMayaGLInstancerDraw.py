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

import os
import sys
import unittest


class testPxrUsdMayaGLInstancerDraw(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        # To control where the rendered images are written, we force Maya to
        # use the test directory as the workspace.
        cmds.workspace(os.path.abspath('.'), o=True)

    def _WriteViewportImage(self, outputImageName, suffix):
        # Make sure the hardware renderer is available
        MAYA_RENDERER_NAME = 'mayaHardware2'
        mayaRenderers = cmds.renderer(query=True,
            namesOfAvailableRenderers=True)
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
        cmds.ogsRender(camera="persp", currentFrame=True, width=960, height=540)

    def _SetModelPanelsToViewport2(self):
        modelPanels = cmds.getPanel(type='modelPanel')
        for modelPanel in modelPanels:
            cmds.modelEditor(modelPanel, edit=True, rendererName='vp2Renderer')

    def testGenerateImages(self):
        cmds.file(os.path.abspath('InstancerDrawTest.ma'),
                open=True, force=True)

        # Draw in VP2 at current frame.
        self._SetModelPanelsToViewport2()
        self._WriteViewportImage("InstancerTest", "initial")

        # Load assembly in "Cards" to use cards drawmode.
        cmds.assembly("CubeModel", edit=True, active="Cards")
        self._WriteViewportImage("InstancerTest", "cards")

        # Change the time; this will change the instancer positions.
        cmds.currentTime(50, edit=True)
        self._WriteViewportImage("InstancerTest", "frame50")

        # Delete the first instance.
        # Tickle the second instance so that it draws.
        # Note: it's OK that we need to tickle; the instancing support is not
        # the greatest and we're just checking that it doesn't break horribly.
        cmds.delete("instance1")
        cmds.setAttr("instance2.t", 15, 0, 0, type="double3")
        self._WriteViewportImage("InstancerTest", "instance2")

        # Delete the second instance.
        cmds.delete("instance2")
        self._WriteViewportImage("InstancerTest", "empty")

        # Reload the scene. We should see the instancer come back up.
        # Hopefully the instancer imager has reloaded in a sane way!
        cmds.file(os.path.abspath('InstancerDrawTest.ma'),
                open=True, force=True)
        self._SetModelPanelsToViewport2()
        self._WriteViewportImage("InstancerTest", "reload")

if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(
            testPxrUsdMayaGLInstancerDraw)

    results = unittest.TextTestRunner(stream=sys.__stderr__).run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1
    # maya running interactively often absorbs all the output.  comment out the
    # following to prevent maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)
