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
import imageUtils
import mayaUtils

from mayaUsd import lib as mayaUsdLib

from pxr import Kind

from maya import cmds
from maya.api import OpenMayaUI as OMUI

import ufe

from PySide2 import QtCore
from PySide2.QtTest import QTest
from PySide2.QtWidgets import QWidget

from shiboken2 import wrapInstance

import os


class testVP2RenderDelegatePointInstancesPickMode(imageUtils.ImageDiffingTestCase):
    """
    Tests viewport picking using different point instances pick modes of the
    Viewport 2.0 render delegate.
    """

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegatePointInstancesPickModeTest', 'baseline')

        cls._testDir = os.path.abspath('.')

        # Store the previous USD point instances pick mode and selection kind
        # (or None if unset) so we can restore the state later.
        cls._pointInstancesPickModeOptionVarName = mayaUsdLib.OptionVarTokens.PointInstancesPickMode
        cls._prevPointInstancesPickMode = cmds.optionVar(
            query=cls._pointInstancesPickModeOptionVarName) or None

        cls._selectionKindOptionVarName = mayaUsdLib.OptionVarTokens.SelectionKind
        cls._prevSelectionKind = cmds.optionVar(
            query=cls._selectionKindOptionVarName) or None

        mayaUtils.openPointInstancesGrid14Scene()

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

    def setUp(self):
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

        # Clear both optionVars before each test.
        cmds.optionVar(remove=self._pointInstancesPickModeOptionVarName)
        cmds.optionVar(remove=self._selectionKindOptionVarName)

    def _ClickInViewCenter(self):
        view = OMUI.M3dView.active3dView()
        viewWidget = wrapInstance(long(view.widget()), QWidget)

        QTest.mouseClick(viewWidget, QtCore.Qt.LeftButton,
            QtCore.Qt.NoModifier, viewWidget.rect().center())

    def assertSnapshotClose(self, imageName):
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=960, height=540)
        return self.assertImagesClose(baselineImage, snapshotImage)

    def _RunTest(self, expectedUfePathString):
        self.assertSnapshotClose('%s_initial.png' % self._testName)

        self._ClickInViewCenter()

        self.assertSnapshotClose('%s_picked.png' % self._testName)

        globalSelection = ufe.GlobalSelection.get()
        self.assertEqual(len(globalSelection), 1)

        sceneItem = globalSelection.front()
        ufePathString = ufe.PathString.string(sceneItem.path())
        self.assertEqual(ufePathString, expectedUfePathString)

    def testUnsetPointInstancesPickMode(self):
        """
        Tests viewport picking with point instances pick mode not specified.
        The default of "PointInstancer" mode should be used.
        """
        self._testName = 'Default'

        expectedUfePathString = (
            '|UsdProxy|UsdProxyShape,'
            '/PointInstancerGrid/PointInstancer')
        self._RunTest(expectedUfePathString)

    def testPointInstancerPointInstancesPickMode(self):
        """
        Tests viewport picking with the default "PointInstancer"
        point instances pick mode specified.

        This should correspond to the "Prims" pick mode in usdview.
        """
        self._testName = 'PointInstancer'

        cmds.optionVar(stringValue=(self._pointInstancesPickModeOptionVarName,
            self._testName))

        expectedUfePathString = (
            '|UsdProxy|UsdProxyShape,'
            '/PointInstancerGrid/PointInstancer')
        self._RunTest(expectedUfePathString)

    def testInstancesPointInstancesPickMode(self):
        """
        Tests viewport picking with the "Instances" point instances pick mode
        specified.

        This should correspond to the "Instances" pick mode in usdview.
        """
        self._testName = 'Instances'

        cmds.optionVar(stringValue=(self._pointInstancesPickModeOptionVarName,
            self._testName))

        expectedUfePathString = (
            '|UsdProxy|UsdProxyShape,'
            '/PointInstancerGrid/PointInstancer/3')
        self._RunTest(expectedUfePathString)

    def testPrototypesPointInstancesPickMode(self):
        """
        Tests viewport picking with the "Prototypes" point instances pick mode
        specified.

        This should correspond to the "Prototypes" pick mode in usdview.
        """
        self._testName = 'Prototypes'

        cmds.optionVar(stringValue=(self._pointInstancesPickModeOptionVarName,
            self._testName))

        expectedUfePathString = (
            '|UsdProxy|UsdProxyShape,'
            '/PointInstancerGrid/PointInstancer/prototypes/GreenCube/Geom/Cube')
        self._RunTest(expectedUfePathString)

    def testUsdviewModelsPickMode(self):
        """
        Tests viewport picking in a configuration that corresponds to usdview's
        "Models" pick mode.
        """
        self._testName = 'Models'

        cmds.optionVar(stringValue=(self._pointInstancesPickModeOptionVarName,
            'Prototypes'))
        cmds.optionVar(stringValue=(self._selectionKindOptionVarName,
            Kind.Tokens.model))

        expectedUfePathString = (
            '|UsdProxy|UsdProxyShape,'
            '/PointInstancerGrid/PointInstancer/prototypes/GreenCube')
        self._RunTest(expectedUfePathString)

    def testPickingNonInstancesWithModelKind(self):
        """
        Tests viewport picking with the "PointInstancer"
        point instances pick mode specified, along with selection kind
        "model".

        The configurability of point instances pick mode and selection kind
        together is more flexible than usdview's pick modes, so this
        configuration cannot be expressed in usdview. The point instances pick
        mode should resolve to the PointInstancer prim, and the selection kind
        should be searched for up the hierarchy starting from there. This
        differs from the usdview-style "Models" pick mode which starts the
        kind search from the prototype.

        The root prim has kind 'component', which IsA model, whereas the
        PointInstancer prim has kind 'subcomponent', which is *not* model.
        """
        self._testName = 'NonInstanceModels'

        cmds.optionVar(stringValue=(self._pointInstancesPickModeOptionVarName,
            'PointInstancer'))
        cmds.optionVar(stringValue=(self._selectionKindOptionVarName,
            Kind.Tokens.model))

        expectedUfePathString = (
            '|UsdProxy|UsdProxyShape,'
            '/PointInstancerGrid')
        self._RunTest(expectedUfePathString)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
