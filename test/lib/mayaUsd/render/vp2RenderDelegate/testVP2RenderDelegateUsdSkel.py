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


class testVP2RenderDelegateUsdSkel(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using per-instance
    inherited data on instances.
    """

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegateUsdSkelTest', 'baseline')

        cls._testDir = os.path.abspath('.')

    def assertSnapshotClose(self, imageName):
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=960, height=540)
        return self.assertImagesClose(baselineImage, snapshotImage)

    def _StartTest(self, testName):
        self._testName = testName
        testFile = testUtils.getTestScene("UsdSkel", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        cmds.currentTime(0)
        self.assertSnapshotClose('%s_0.png' % self._testName)
        cmds.currentTime(50)
        self.assertSnapshotClose('%s_50.png' % self._testName)
        cmds.currentTime(100)
        self.assertSnapshotClose('%s_100.png' % self._testName)

    def testPerInstanceInheritedData(self):
        mayaUtils.loadPlugin("mayaUsdPlugin")

        cmds.file(force=True, new=True)
        cmds.move(-8, 15, 6, 'persp')
        cmds.rotate(70, 0, -160, 'persp')
        self._StartTest('skinCluster')

        cmds.file(force=True, new=True)
        cmds.move(180, -525, 225, 'persp')
        cmds.rotate(75, 0, 0, 'persp')
        self._StartTest('HIK_Export')



if __name__ == '__main__':
    fixturesUtils.runTests(globals())
