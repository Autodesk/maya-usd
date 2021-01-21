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

from maya import cmds

import os
import sys
import unittest

import fixturesUtils


class testProxyShapeDrawColors(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False)

        cls._testName = 'ProxyShapeDrawColorsTest'
        cls._inputDir = os.path.join(inputPath, cls._testName)

        cls._testDir = os.path.abspath('.')

    def setUp(self):
        mayaSceneFile = '%s.ma' % self._testName
        mayaSceneFullPath = os.path.join(self._inputDir, mayaSceneFile)
        cmds.file(mayaSceneFullPath, open=True, force=True)

        # To control where the rendered images are written, we force Maya to
        # use the test directory as the workspace.
        cmds.workspace(self._testDir, o=True)

    def testMeshNextToProxyShapeAndImported(self):
        """
        Tests the colors between a mesh with (0.55, 0.55, 0.55)
        exporting that file and then re-importing it, and also referencing
        it back into the same scene through a proxy shape.

        While this is a bit more than just "GL" testing, it's a useful place to
        centralize all this.  If we don't like that this is testing usdImport
        functionality, we can remove

        This will render as follows:


        blank    |  usdImport
                 |
        ---------+-----------
        modeled  |  ref'd
                 |

        """
        x = self._PlaneWithColor((0.55, 0.55, 0.55))
        cmds.select(x)
        usdFile = os.path.join(self._testDir, 'plane.usd')
        cmds.mayaUSDExport(file=usdFile, selection=True, shadingMode='none',
            exportDisplayColor=True)
        proxyShape = cmds.createNode('mayaUsdProxyShape', name='usdProxyShape')
        proxyTransform = cmds.listRelatives(proxyShape, parent=True,
            fullPath=True)[0]
        cmds.xform(proxyTransform, translation=(30.48, 0, 0))
        cmds.setAttr('%s.filePath' % proxyShape, usdFile, type='string')
        cmds.setAttr('%s.primPath' % proxyShape, '/pPlane1', type='string')

        x = cmds.mayaUSDImport(file=usdFile)
        cmds.xform(x, translation=(30.48, 30.48, 0))

        cmds.setAttr("hardwareRenderingGlobals.floatingPointRTEnable", 0)
        cmds.setAttr('defaultColorMgtGlobals.outputTransformEnabled', 0)
        self._Snapshot('default')
        cmds.setAttr("hardwareRenderingGlobals.floatingPointRTEnable", 1)
        cmds.setAttr('defaultColorMgtGlobals.outputTransformEnabled', 1)
        self._Snapshot('colorMgt')

    def _PlaneWithColor(self, c):
        ret = cmds.polyPlane(width=30.48, height=30.48, sx=4, sy=4, ax=(0, 0, 1))
        cmds.setAttr('lambert1.color', *c)
        return ret

    def _Snapshot(self, outName):
        cmds.select([])
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

        cmds.setAttr('defaultRenderGlobals.imageFilePrefix', outName,
                type='string')

        # Do the render.
        cmds.ogsRender(camera='top', currentFrame=True, width=400, height=400)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
