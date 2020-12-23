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

from pxr import Gf

from maya import OpenMayaUI as OMUI
from maya import cmds

from PySide2 import QtCore
from PySide2.QtTest import QTest
from PySide2.QtWidgets import QWidget

from shiboken2 import wrapInstance

import os
import sys
import unittest

import fixturesUtils


class testProxyShapeLiveSurface(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        inputPath = fixturesUtils.readOnlySetUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._testName = 'ProxyShapeLiveSurfaceTest'
        cls._inputDir = os.path.join(inputPath, cls._testName)

    def setUp(self):
        mayaSceneFile = '%s.ma' % self._testName
        mayaSceneFullPath = os.path.join(self._inputDir, mayaSceneFile)
        cmds.file(mayaSceneFullPath, open=True, force=True)

        # Create a new custom viewport.
        self._window = cmds.window(widthHeight=(500, 400))
        cmds.paneLayout()
        self._panel = cmds.modelPanel()
        cmds.modelPanel(self._panel, edit=True, camera='persp')
        cmds.modelEditor(cmds.modelPanel(self._panel, q=True, modelEditor=True),
            edit=True, displayAppearance='smoothShaded', rnm='vp2Renderer')
        cmds.showWindow(self._window)

        # Force all views to re-draw. This causes us to block until the
        # geometry we're making live has actually been evaluated and is present
        # in the viewport. Otherwise execution of the test may continue and the
        # create tool may be invoked before there's any geometry there, in
        # which case the tool won't create anything.
        cmds.refresh()

        # Get the viewport widget.
        self._view = OMUI.M3dView()
        OMUI.M3dView.getM3dViewFromModelPanel(self._panel, self._view)
        self._viewWidget = wrapInstance(long(self._view.widget()), QWidget)

    def tearDown(self):
        self._viewWidget = None
        self._view = None
        cmds.deleteUI(self._window)

    def testObjectPosition(self):
        """
        Tests that an object created interactively is positioned correctly on
        the live surface.
        """

        # Make our proxy shape live.
        cmds.makeLive('Block_1')

        # Enter interactive creation context.
        cmds.setToolTo('CreatePolyTorusCtx')

        # Click in the center of the viewport widget.
        QTest.mouseClick(self._viewWidget, QtCore.Qt.LeftButton,
            QtCore.Qt.NoModifier, self._viewWidget.rect().center())

        # Find the torus (it should be called pTorus1).
        self.assertTrue(cmds.ls('pTorus1'))

        # Check the torus's transform.
        # The Block_1 is originally 1 unit deep and centered at the origin, so
        # its original top plane is aligned at z=0.5.
        # It is scaled 5x, which makes the top plane aligned at z=2.5.
        # Then it is translated along z by 4.0, so its top plane is finally
        # aligned at z=6.5.
        translate = cmds.xform('pTorus1', q=True, t=True)
        self.assertAlmostEqual(translate[2], 6.5, places=3)

    def testObjectNormal(self):
        """
        Tests that an object created interactively by dragging in the viewport
        has the correct orientation based on the live surface normal.
        """

        # Make our proxy shape live.
        cmds.makeLive('Block_2')

        # Enter interactive creation context.
        cmds.setToolTo('CreatePolyConeCtx')

        # Click in the center of the viewport widget.
        QTest.mouseClick(self._viewWidget, QtCore.Qt.LeftButton,
            QtCore.Qt.NoModifier, self._viewWidget.rect().center())

        # Find the cone (it should be called pCone1).
        self.assertTrue(cmds.ls('pCone1'))

        # Check the cone's rotation.
        # Because our scene is Z-axis up, the cone's Z-axis should be aligned
        # with Block_2's surface normal (though it might not necessarily have
        # the same exact rotation).
        rotationAngles = cmds.xform('pCone1', q=True, ro=True)
        rotation = (Gf.Rotation(Gf.Vec3d.XAxis(), rotationAngles[0]) *
            Gf.Rotation(Gf.Vec3d.YAxis(), rotationAngles[1]) *
            Gf.Rotation(Gf.Vec3d.ZAxis(), rotationAngles[2]))
        actualZAxis = rotation.TransformDir(Gf.Vec3d.ZAxis())

        expectedRotation = (Gf.Rotation(Gf.Vec3d.XAxis(), 75.0) *
            Gf.Rotation(Gf.Vec3d.YAxis(), 90.0))
        expectedZAxis = expectedRotation.TransformDir(Gf.Vec3d.ZAxis())

        # Verify that the error angle between the two axes is less than
        # 0.1 degrees (less than ~0.0003 of a revolution, so not bad). That's
        # about as close as we're going to get.
        errorRotation = Gf.Rotation(actualZAxis, expectedZAxis)
        self.assertLess(errorRotation.GetAngle(), 0.1)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
