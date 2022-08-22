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

import ufe

import os


class testVP2RenderDelegateDuplicateProxy(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using duplicate
    proxy shapes which share a USD stage
    """

    @classmethod
    def setUpClass(cls):
        # The baselines assume Y-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='y')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegateDuplicateProxyTest', 'baseline')

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
        # Re-using the test scene from the consolidation test
        testFile = testUtils.getTestScene("consolidation", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

    def testDuplicateProxy(self):
        self._StartTest('colorConsolidation')

        # Set up the test
        cmds.duplicate('stage')
        cmds.move(0, -5, 0, 'stage1', relative=True)
        self.assertSnapshotClose('%s_duplicate.png' % self._testName)

        cmds.select("|stage|stageShape,/pCone1")
        cmds.move(-5, 0, 0, relative=True)
        cmds.select("|stage1|stage1Shape,/pCone1")
        self.assertSnapshotClose('%s_duplicate_AfterMove.png' % self._testName)




if __name__ == '__main__':
    fixturesUtils.runTests(globals())
