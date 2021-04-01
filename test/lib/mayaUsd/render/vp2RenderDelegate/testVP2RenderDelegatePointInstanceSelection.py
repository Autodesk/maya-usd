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
import usdUtils

from mayaUsd import lib as mayaUsdLib
from mayaUsd import ufe as mayaUsdUfe

from maya import cmds

import ufe

import os


class testVP2RenderDelegatePointInstanceSelection(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when selecting
    instances of a PointInstancer.
    """

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
        ufePath = testVP2RenderDelegatePointInstanceSelection._GetUfePath(
            instanceIndex)
        ufeItem = ufe.Hierarchy.createItem(ufePath)
        return ufeItem

    def assertSnapshotClose(self, imageName):
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=960, height=540)
        return self.assertImagesClose(baselineImage, snapshotImage)

    def _RunTest(self):
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertSnapshotClose('%s_unselected.png' % self._testName)

        # Select one instance.
        sceneItem = self._GetSceneItem(0)
        globalSelection.append(sceneItem)
        self.assertSnapshotClose('%s_select_one.png' % self._testName)
        globalSelection.clear()

        # We'll populate a new selection and swap that into the global
        # selection to minimize the overhead of modifying the global selection
        # one item at a time.
        newSelection = ufe.Selection()

        # Select the first seven instances. The most recently selected item
        # should get "Lead" highlighting.
        for instanceIndex in range(7):
            sceneItem = self._GetSceneItem(instanceIndex)
            newSelection.append(sceneItem)
        globalSelection.replaceWith(newSelection)
        self.assertSnapshotClose('%s_select_seven.png' % self._testName)
        globalSelection.clear()
        newSelection.clear()

        # Select the back half of the instances.
        for instanceIndex in range(self._numInstances // 2, self._numInstances):
            sceneItem = self._GetSceneItem(instanceIndex)
            newSelection.append(sceneItem)
        globalSelection.replaceWith(newSelection)
        self.assertSnapshotClose('%s_select_half.png' % self._testName)
        globalSelection.clear()
        newSelection.clear()

        # Select all instances
        for instanceIndex in range(self._numInstances):
            sceneItem = self._GetSceneItem(instanceIndex)
            newSelection.append(sceneItem)
        globalSelection.replaceWith(newSelection)
        self.assertSnapshotClose('%s_select_all.png' % self._testName)
        globalSelection.clear()
        newSelection.clear()

        # Select the PointInstancer itself
        sceneItem = self._GetSceneItem()
        globalSelection.append(sceneItem)
        self.assertSnapshotClose('%s_select_PointInstancer.png' % self._testName)
        globalSelection.clear()

    def testPointInstancerGrid14(self):
        self._numInstances = 14
        self._testName = 'Grid_14'
        mayaUtils.openPointInstancesGrid14Scene()
        self._RunTest()

    def testPointInstancerGrid7k(self):
        self._numInstances = 7000
        self._testName = 'Grid_7k'
        mayaUtils.openPointInstancesGrid7kScene()
        self._RunTest()

    def testPointInstancerGrid70k(self):
        self._numInstances = 70000
        self._testName = 'Grid_70k'
        mayaUtils.openPointInstancesGrid70kScene()
        self._RunTest()


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
