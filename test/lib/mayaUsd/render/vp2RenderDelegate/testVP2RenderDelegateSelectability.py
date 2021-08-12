#!/usr/bin/env mayapy
#
# Copyright 2016 Pixar
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
from maya.api import OpenMayaUI as OMUI
from pxr import Usd, Sdf

import fixturesUtils
import mayaUtils
import testUtils

import os
import unittest

import ufe
import mayaUsd.ufe

from PySide2 import QtCore
from PySide2.QtTest import QTest
from PySide2.QtWidgets import QWidget

from shiboken2 import wrapInstance

class testVP2RenderDelegateSelectability(unittest.TestCase):

    selectabilityToken = "maya_selectability"
    onToken = "on"
    offToken = "off"
    inheritToken = "inherit"

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__, initializeStandalone=False)

    @classmethod
    def tearDownClass(cls):
        pass

    def _processViewEvents(self, timeout=10):
        '''
        Helper that forces Maya to process events.
        '''
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, timeout)

    def _dragSelectActiveView(self):
        '''
        Helper that drags-select (region-select) the whole viewport.
        '''
        view = OMUI.M3dView.active3dView()
        viewWidget = wrapInstance(int(view.widget()), QWidget)

        viewWidget.update()
        self._processViewEvents()

        QTest.mousePress(viewWidget, QtCore.Qt.LeftButton,
                    QtCore.Qt.NoModifier, viewWidget.rect().topLeft() + QtCore.QPoint(1, 1))
        QTest.mouseMove(viewWidget, viewWidget.rect().bottomRight() - QtCore.QPoint(1,1))
        QTest.mouseRelease(viewWidget, QtCore.Qt.LeftButton,
            QtCore.Qt.NoModifier, viewWidget.rect().bottomRight() - QtCore.QPoint(1, 1))

    def _createLayer(self):
        '''
        Helper that creates a layer and return the item of its shape item.
        '''
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)

        return proxyShapeItem

    def _createPrim(self, underItem, primType, expectedPath):
        '''
        Helper that creates a prim of the given type under the given item, and return its item.
        '''
        # create a proxy shape and add a Cone prim

        proxyShapeContextOps = ufe.ContextOps.contextOps(underItem)
        proxyShapeContextOps.doOp(['Add New Prim', primType])

        primPath = ufe.Path([
            ufe.PathString.path("|stage1|stageShape1").segments[0],
            ufe.PathSegment(expectedPath, mayaUsd.ufe.getUsdRunTimeId(), "/")
            ])
        primItem = ufe.Hierarchy.createItem(primPath)
        primRawItem = primItem.getRawAddress()
        prim = mayaUsd.ufe.getPrimFromRawItem(primRawItem)

        return prim, primItem

    def testSelectability(self):
        '''
        Verify that a prim is selectable and detected as such when its selectability is set to self.onToken.
        '''
        cmds.file(new=True, force=True)

        layerItem = self._createLayer()
        conePrim, _ = self._createPrim(layerItem, 'Cone', 'Cone1')
        conePrim.SetMetadata(self.selectabilityToken, self.onToken)

        self._dragSelectActiveView()

        # verify there is the cone in the selection
        sel = ufe.GlobalSelection.get()
        self.assertEqual(1, len(list(iter(sel))))
        snIter = iter(sel)
        coneItem = next(snIter)
        self.assertEqual(str(coneItem.nodeName()), "Cone1")

    def testUnselectability(self):
        '''
        Verify that a prim is *not* selectable and detected as such when its selectability is set to self.offToken.
        '''
        cmds.file(new=True, force=True)

        layerItem = self._createLayer()
        conePrim, _ = self._createPrim(layerItem, 'Cone', 'Cone1')
        conePrim.SetMetadata(self.selectabilityToken, self.offToken)

        self._dragSelectActiveView()

        # verify there is nothing in the selection
        self.assertEqual(0, len(list(iter(ufe.GlobalSelection.get()))))

    def testSelectabilityUnderUnselectability(self):
        '''
        Verify that a selectable prim under a non-selectable prim is selectable.
        '''
        cmds.file(new=True, force=True)

        layerItem = self._createLayer()
        conePrim, coneItem = self._createPrim(layerItem, 'Cone', 'Cone1')
        conePrim.SetMetadata(self.selectabilityToken, self.offToken)
        cubePrim, _ = self._createPrim(coneItem, 'Cube', 'Cone1/Cube1')
        cubePrim.SetMetadata(self.selectabilityToken, self.onToken)

        self._dragSelectActiveView()

        # verify there is the cone in the selection
        sel = ufe.GlobalSelection.get()
        self.assertEqual(1, len(list(iter(sel))))
        snIter = iter(sel)
        coneItem = next(snIter)
        self.assertEqual(str(coneItem.nodeName()), "Cube1")

    def testUnselectabilityUnderSelectability(self):
        '''
        Verify that a unselectable prim under a selectable prim is unselectable.
        '''
        cmds.file(new=True, force=True)

        layerItem = self._createLayer()
        conePrim, coneItem = self._createPrim(layerItem, 'Cone', 'Cone1')
        conePrim.SetMetadata(self.selectabilityToken, self.onToken)
        cubePrim, _ = self._createPrim(coneItem, 'Cube', 'Cone1/Cube1')
        cubePrim.SetMetadata(self.selectabilityToken, self.offToken)

        self._dragSelectActiveView()

        # verify there is the cone in the selection
        sel = ufe.GlobalSelection.get()
        self.assertEqual(1, len(list(iter(sel))))
        snIter = iter(sel)
        coneItem = next(snIter)
        self.assertEqual(str(coneItem.nodeName()), "Cone1")

    def testInheritUnderUnselectability(self):
        '''
        Verify that a inherit prim under a non-selectable prim is unselectable.
        '''
        cmds.file(new=True, force=True)

        layerItem = self._createLayer()
        conePrim, coneItem = self._createPrim(layerItem, 'Cone', 'Cone1')
        conePrim.SetMetadata(self.selectabilityToken, self.offToken)
        cubePrim, _ = self._createPrim(coneItem, 'Cube', 'Cone1/Cube1')
        cubePrim.SetMetadata(self.selectabilityToken, self.inheritToken)

        self._dragSelectActiveView()

        # verify there is the cone in the selection
        sel = ufe.GlobalSelection.get()
        self.assertEqual(0, len(list(iter(sel))))

    def testLayerSelectable(self):
        '''
        Verify that a higher priority layer can make an unselectable prim selectable.
        '''
        cmds.file(new=True, force=True)

        testFile = testUtils.getTestScene("selectability", "cone-layer-selectable-3.usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

        self._dragSelectActiveView()

        # verify there is the cone in the selection
        sel = ufe.GlobalSelection.get()
        self.assertEqual(1, len(list(iter(sel))))
        snIter = iter(sel)
        coneItem = next(snIter)
        self.assertEqual(str(coneItem.nodeName()), "Cone1")

    def testLayerUnselectable(self):
        '''
        Verify that a higher priority layer can make an selectable prim unselectable.
        '''
        cmds.file(new=True, force=True)

        testFile = testUtils.getTestScene("selectability", "cone-layer-unselectable-3.usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

        self._dragSelectActiveView()

        # verify there is the cone in the selection
        sel = ufe.GlobalSelection.get()
        self.assertEqual(0, len(list(iter(sel))))


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
