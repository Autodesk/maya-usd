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
import time

from PySide2 import QtCore
from PySide2.QtTest import QTest
from PySide2.QtWidgets import QWidget

from shiboken2 import wrapInstance

import os


class testVP2RenderDelegatePointInstancesPickMode(imageUtils.ImageDiffingTestCase):
    """
    Reproduce a crash when manipulating a point instanced object
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

    def testCrash(self):
        """
        Move a point instanced object in a loop until we crash!
        """
        self._testName = 'Instances'

        cmds.optionVar(stringValue=(self._pointInstancesPickModeOptionVarName,
            self._testName))

        expectedUfePathString = (
            '|UsdProxy|UsdProxyShape,'
            '/PointInstancerGrid/PointInstancer/3')
        cmds.select(expectedUfePathString)
        #time.sleep(5) #attach the debugger!
        for i in range(1, 300):
            cmds.move(0, 0.1, 0, relative=True)
            cmds.refresh(force=True)



if __name__ == '__main__':
    fixturesUtils.runTests(globals())
