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
import unittest
import usdUtils
import testUtils

from mayaUsd import lib as mayaUsdLib
from mayaUsd import ufe as mayaUsdUfe

from maya import cmds
import maya.api.OpenMayaRender as omr

from pxr import Usd

import ufe

import os


class testVP2RenderDelegatePerInstanceInheritedData(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using per-instance
    inherited data on instances.
    """

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        # cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegatePerInstanceInheritedDataTest', 'baseline')

        cls._testDir = os.path.abspath('.')

    @classmethod
    def tearDownClass(cls):
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, useDefaultMaterial=False)

    def assertSnapshotClose(self, imageName, usdVersion=None):
        paths = []
        if (usdVersion):
            paths = [usdVersion, imageName]
        else:
            paths = [imageName]
        
        baselineImage = os.path.join(self._baselineDir, *paths)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=960, height=540)
        return self.assertImagesClose(baselineImage, snapshotImage)

    def _StartTest(self, testName):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, useDefaultMaterial=False)
        self._testName = testName
        testFile = testUtils.getTestScene("instances", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertSnapshotClose('%s_unselected.png' % self._testName)

    def testPerInstanceInheritedData(self):
        self._StartTest('perInstanceInheritedData')

        # These tests don't work in earlier versions of USD, the wrong
        # instance index gets selected
        usdVersion = Usd.GetVersion()
        if usdVersion < (0, 20, 8):
            return

        # Hide and show some instances to make sure it updates correctly
        stage = mayaUsdUfe.getStage("|stage|stageShape")
        ball_03_vis = stage.GetPrimAtPath('/root/group/ball_03').GetAttribute('visibility')
        ball_04_vis = stage.GetPrimAtPath('/root/group/ball_04').GetAttribute('visibility')

        cmds.select("|stage|stageShape,/root/group/ball_03")
        self.assertSnapshotClose('%s_ball_03_selected.png' % self._testName)

        ball_03_vis.Set('hidden')
        self.assertSnapshotClose('%s_ball_03_hidden.png' % self._testName)
        ball_04_vis.Set('hidden')
        self.assertSnapshotClose('%s_ball_03_and_04_hidden.png' % self._testName)
        ball_03_vis.Set('inherited') # this should show the object again
        self.assertSnapshotClose('%s_ball_04_hidden.png' % self._testName)
        ball_04_vis.Set('inherited')
        self.assertSnapshotClose('%s_shown_after_hidden.png' % self._testName)

        # These tests behave differently before USD version 21.05, so don't run
        # them for those earlier versions.
        if usdVersion < (0, 21, 5):
            return

        imageVersion = None
        if usdVersion < (0, 21, 8):
            imageVersion = 'pre-21_08'

        # Modify the purpose of some instances to make sure they are shown and hidden properly
        ball_03_purpose = stage.GetPrimAtPath('/root/group/ball_03').GetAttribute('purpose')
        ball_04_purpose = stage.GetPrimAtPath('/root/group/ball_04').GetAttribute('purpose')

        ball_03_purpose.Set('guide')
        self.assertSnapshotClose('%s_ball_03_guide.png' % self._testName, imageVersion)
        ball_04_purpose.Set('guide')
        self.assertSnapshotClose('%s_ball_03_and_04_guide.png' % self._testName, imageVersion)
        ball_03_purpose.Set('default')
        self.assertSnapshotClose('%s_ball_04_guide.png' % self._testName, imageVersion)
        ball_04_purpose.Set('default')
        self.assertSnapshotClose('%s_default_after_guide.png' % self._testName, imageVersion)
    
    def testPerInstanceInheritedDataPartialOverridePxrMtls(self):
        self._StartTest('inheritedDisplayColor_noPxrMtls')

    def testPerInstanceInheritedDataPartialOverride(self):
        self._StartTest('inheritedDisplayColor_pxrSurface')

    def testPerInstanceInheriedDataBasisCurves(self):
        self._StartTest('basisCurveInstance')
        cmds.select("|stage|stageShape,/instanced_2")
        self.assertSnapshotClose('%s_selected.png' % self._testName)

    @unittest.skipUnless("SkipWhenDefaultMaterialActive" in dir(omr.MRenderItem), "Requires new SDK API")
    def testInstanceDefaultMaterial(self):
        self._StartTest('defaultMaterialBillboards')
        cmds.select("|stage|stageShape,/root/group/billboard_03",
                    "|stage|stageShape,/root/group/flatquad_03")
        self.assertSnapshotClose('%s_selected.png' % self._testName)
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, useDefaultMaterial=True)
        self.assertSnapshotClose('%s_default.png' % self._testName)
        cmds.select("|stage|stageShape,/root/group/billboard_04",
                    "|stage|stageShape,/root/group/flatquad_04")
        self.assertSnapshotClose('%s_defaultSelected.png' % self._testName)
        cmds.modelEditor(panel, edit=True, useDefaultMaterial=False)
        self.assertSnapshotClose('%s_notDefault.png' % self._testName)

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
