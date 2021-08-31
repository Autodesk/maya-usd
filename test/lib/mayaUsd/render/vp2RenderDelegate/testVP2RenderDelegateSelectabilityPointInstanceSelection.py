#!/usr/bin/env mayapy
#
# Copyright 2021 Autodesk
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

import fixturesUtils
import mayaUtils
import usdUtils

from mayaUsd import lib as mayaUsdLib
from mayaUsd import ufe as mayaUsdUfe

from maya import cmds
from maya.api import OpenMayaUI as OMUI

from pxr import Usd

import ufe

import os
import unittest

from PySide2 import QtCore
from PySide2.QtTest import QTest
from PySide2.QtWidgets import QWidget

from shiboken2 import wrapInstance

class testVP2RenderDelegateSelectabilityPointInstanceSelection(unittest.TestCase):
    """
    Tests selectability using the Viewport 2.0 render delegate when selecting
    instances of a PointInstancer.
    """

    selectabilityToken = "mayaSelectability"
    onToken = "on"
    offToken = "off"
    inheritToken = "inherit"

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegatePointInstanceSelectionTest', 'baseline')

        cls._testDir = os.path.abspath('.')

        # Store the previous USD point instances pick mode and selection kind
        # (or None if unset) so we can restore the state later.
        cls._pointInstancesPickModeOptionVarName = mayaUsdLib.OptionVarTokens.PointInstancesPickMode
        cls._prevPointInstancesPickMode = cmds.optionVar(
            query=cls._pointInstancesPickModeOptionVarName) or None

        cls._selectionKindOptionVarName = mayaUsdLib.OptionVarTokens.SelectionKind
        cls._prevSelectionKind = cmds.optionVar(
            query=cls._selectionKindOptionVarName) or None

        # Set the USD point instances pick mode to "Instances" so that we pick
        # individual point instances during the test.
        cmds.optionVar(stringValue=(
            cls._pointInstancesPickModeOptionVarName, 'Instances'))

        # Clear any setting for selection kind.
        cmds.optionVar(remove=cls._selectionKindOptionVarName)

    @classmethod
    def tearDownClass(cls):
        # Restore the previous USD point instances pick mode and selection
        # kind, or remove if they were unset.
        if cls._prevPointInstancesPickMode is None:
            cmds.optionVar(remove=cls._pointInstancesPickModeOptionVarName)
        else:
            cmds.optionVar(stringValue=
                (cls._pointInstancesPickModeOptionVarName,
                    cls._prevPointInstancesPickMode))

        if cls._prevSelectionKind is None:
            cmds.optionVar(remove=cls._selectionKindOptionVarName)
        else:
            cmds.optionVar(stringValue=
                (cls._selectionKindOptionVarName, cls._prevSelectionKind))

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

    @staticmethod
    def _GetUfePath(instanceIndex=-1):
        mayaSegment = mayaUtils.createUfePathSegment('|UsdProxy|UsdProxyShape')
        usdSegmentString = mayaUsdUfe.usdPathToUfePathSegment(
            '/PointInstancerGrid/PointInstancer', instanceIndex)
        usdSegment = usdUtils.createUfePathSegment(usdSegmentString)
        ufePath = ufe.Path([mayaSegment, usdSegment])
        return ufePath

    @staticmethod
    def _GetSceneItem(instanceIndex=-1):
        ufePath = testVP2RenderDelegateSelectabilityPointInstanceSelection._GetUfePath(
            instanceIndex)
        ufeItem = ufe.Hierarchy.createItem(ufePath)
        prim = mayaUsdUfe.ufePathToPrim(ufe.PathString.string(ufePath))
        return ufeItem, prim

    def _RunTest(self, expectedCount):
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertTrue(globalSelection.empty())

        self._dragSelectActiveView()

        globalSelection = ufe.GlobalSelection.get()
        self.assertEqual(expectedCount, len(globalSelection))

        # Make the instancer unselectable.
        sceneItem, prim = self._GetSceneItem(0)
        prim.SetMetadata(self.selectabilityToken, self.offToken)

        self._dragSelectActiveView()

        globalSelection = ufe.GlobalSelection.get()
        self.assertTrue(globalSelection.empty())

    def testInstancesGrid14(self):
        mayaUtils.openPointInstancesGrid14Scene()
        cmds.FrameAllInAllViews()
        # Set the USD point instances pick mode to "Instances" so that we pick
        # individual point instances during the test.
        cmds.optionVar(stringValue=(
            testVP2RenderDelegateSelectabilityPointInstanceSelection._pointInstancesPickModeOptionVarName, 'Instances'))
        self._RunTest(14)

    def testInstancesGrid7k(self):
        mayaUtils.openPointInstancesGrid7kScene()
        cmds.FrameAllInAllViews()
        # Set the USD point instances pick mode to "Instances" so that we pick
        # individual point instances during the test.
        cmds.optionVar(stringValue=(
            testVP2RenderDelegateSelectabilityPointInstanceSelection._pointInstancesPickModeOptionVarName, 'Instances'))
        self._RunTest(7000)

    def testPointInstancerGrid14(self):
        mayaUtils.openPointInstancesGrid14Scene()
        cmds.FrameAllInAllViews()
        # Set the USD point instances pick mode to "PointInstancer" so that we pick
        # the instancer and not point instances during the test.
        cmds.optionVar(stringValue=(
            testVP2RenderDelegateSelectabilityPointInstanceSelection._pointInstancesPickModeOptionVarName, 'PointInstancer'))

        # In USD versions before 21.05, the point instancer pick mode did not exists.
        # For those version we end-up selecting the prototypes, of which there are 7.
        expectedCount = 1 if Usd.GetVersion() >= (0, 21, 5) else 7

        self._RunTest(expectedCount)

    def testPointInstancerGrid7k(self):
        mayaUtils.openPointInstancesGrid7kScene()
        cmds.FrameAllInAllViews()
        # Set the USD point instances pick mode to "PointInstancer" so that we pick
        # the instancer and not point instances instances during the test.
        cmds.optionVar(stringValue=(
            testVP2RenderDelegateSelectabilityPointInstanceSelection._pointInstancesPickModeOptionVarName, 'PointInstancer'))

        # In USD versions before 21.05, the point instancer pick mode did not exists.
        # For those version we end-up selecting the prototypes, of which there are 7.
        expectedCount = 1 if Usd.GetVersion() >= (0, 21, 5) else 7
        
        self._RunTest(expectedCount)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
