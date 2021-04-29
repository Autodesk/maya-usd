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
import testUtils

from mayaUsd import lib as mayaUsdLib
from mayaUsd import ufe as mayaUsdUfe

from maya import cmds

from pxr import Vt

import ufe

import os


class testVP2RenderDelegateConsolidation(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using consolidation
    """

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        # cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegateConsolidationTest', 'baseline')

        cls._testDir = os.path.abspath('.')

    def assertSnapshotClose(self, imageName):
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=960, height=540)
        return self.assertImagesClose(baselineImage, snapshotImage)

    def _StartTest(self, testName):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        self._testName = testName
        testFile = testUtils.getTestScene("consolidation", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

    def testModifyDisplayColor(self):
        self._StartTest('capsule')

        cmds.move(3, 3, 3, 'persp')
        cmds.rotate(-30, 45, 0, 'persp')
        self.assertSnapshotClose('%s_unselected.png' % self._testName)

        stage = mayaUsdUfe.getStage("|stage|stageShape")
        rprim = stage.GetPrimAtPath('/Capsule1')
        displayColorPrimvar = rprim.GetAttribute('primvars:displayColor')
        displayColorPrimvar.Set(Vt.Vec3fArray([(0.0, 1.0, 0.0)]))
        self.assertSnapshotClose('%s_green.png' % self._testName)

    def testModifyConsolidatedMaterial(self):
        self._StartTest('colorConsolidation')
        self.assertSnapshotClose('%s_unselected.png' % self._testName)

        stage = mayaUsdUfe.getStage("|stage|stageShape")
        shader = stage.GetPrimAtPath('/lambert2SG/sphereLambert')
        diffuseColorAttr = shader.GetAttribute('inputs:diffuseColor')
        diffuseColorAttr.Set((0.0, 1.0, 0.0))

        self.assertSnapshotClose('%s_greenSpheres.png' % self._testName)



if __name__ == '__main__':
    fixturesUtils.runTests(globals())
