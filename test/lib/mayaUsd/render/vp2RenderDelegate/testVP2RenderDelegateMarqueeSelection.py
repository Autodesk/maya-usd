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

from maya import cmds
from maya.api import OpenMayaUI as OMUI

import fixturesUtils

import unittest

import ufe
import mayaUtils

try:
    from PySide2 import QtCore
    from PySide2.QtWidgets import QWidget
    from shiboken2 import wrapInstance
except Exception:
    from PySide6 import QtCore
    from PySide6.QtWidgets import QWidget
    from shiboken6 import wrapInstance

import qtInputHelpers


class testVP2RenderDelegateMarqueeSelection(unittest.TestCase):
    """
    Tests marquee selection behaviour with proxyShapes.
    """
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        cmds.file(new=True, force=True)

    def _processViewEvents(self, timeout=10):
        '''
        Helper that forces Maya to process events.
        '''
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.ProcessEventsFlag.AllEvents, timeout)

    def _dragSelectActiveView(self):
        '''
        Helper that drags-select (region-select) the whole viewport.
        '''
        view = OMUI.M3dView.active3dView()
        viewWidget = wrapInstance(int(view.widget()), QWidget)

        viewWidget.update()
        self._processViewEvents()

        top_left = viewWidget.rect().topLeft() + QtCore.QPoint(1, 1)
        bottom_right = viewWidget.rect().bottomRight() - QtCore.QPoint(1, 1)
        qtInputHelpers.send_mouse_drag(viewWidget, top_left, bottom_right)

    def _createProxyWithCube(self, translation, enableUfeSelection):
        '''
        Create a proxy shape holding a single cube prim.
        Returns the UFE path of what a viewport selection is expected to pick.
        '''
        proxyShape, stage = mayaUtils.createProxyAndStage()
        stage.DefinePrim('/cube', 'Cube')

        transform = cmds.listRelatives(proxyShape, parent=True, fullPath=True)[0]
        cmds.xform(transform, translation=translation)
        cmds.setAttr(f'{proxyShape}.enableUfeSelection', enableUfeSelection)

        return f'{proxyShape},/cube' if enableUfeSelection else transform

    def _createMultipleProxiesWithCube(self, enableUfeSelection, translateY=0.0):
        '''
        Create three proxy shapes side by side, on the given row.
        Returns the UFE paths of what a viewport selection is expected to pick.
        '''
        return [
            self._createProxyWithCube((translateX, translateY, 0), enableUfeSelection)
            for translateX in (-4.0, 0.0, 4.0)
        ]

    def _verifyMarqueeSelectsEveryUfePath(self, expectedUfePaths):
        '''
        Marquee-select the whole viewport and compare the global selection with the expected paths.
        '''
        globalSel = ufe.GlobalSelection.get()
        globalSel.clear()

        cmds.viewFit(all=True)
        self._processViewEvents()
        self._dragSelectActiveView()

        selected = [ufe.PathString.string(item.path()) for item in globalSel]
        self.assertEqual(sorted(selected), sorted(expectedUfePaths))

    def testMarqueeSelectsEveryProxyShape(self):
        '''
        A rectangle over several proxy shapes must select them all.
        It used to select only one proxyShape with ufeSelectionEnabled turned off.
        '''
        selectable = self._createMultipleProxiesWithCube(enableUfeSelection=False)

        self._verifyMarqueeSelectsEveryUfePath(selectable)

    def testMarqueeSelectsEveryUfePrim(self):
        '''
        A rectangle over proxyShapes with ufeSelectionEnabled turned on must select
        all the prims.
        '''
        selectable = self._createMultipleProxiesWithCube(enableUfeSelection=True)

        self._verifyMarqueeSelectsEveryUfePath(selectable)

    def testMarqueeSelectsMixedItems(self):
        '''
        A rectangle over proxyShapes in both ufeSelection modes and a native Maya mesh
        must select them all.
        '''
        selectable = self._createMultipleProxiesWithCube(
            enableUfeSelection=False, translateY=-2.0)

        selectable += self._createMultipleProxiesWithCube(
            enableUfeSelection=True, translateY=2.0)

        mayaCube = cmds.polyCube(name='mayaCube')[0]
        cmds.setAttr(f'{mayaCube}.translateY', 8.0)
        selectable.append(cmds.ls(mayaCube, long=True)[0])

        self._verifyMarqueeSelectsEveryUfePath(selectable)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
