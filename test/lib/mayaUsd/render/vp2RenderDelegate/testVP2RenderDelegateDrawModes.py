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

from pxr import Gf

from maya import cmds

import ufe
import sys
import os


class testVP2RenderDelegateDrawModes(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using usd cameras
    """

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        # cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegateDrawModesTest', 'baseline')

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
        testFile = testUtils.getTestScene("drawModes", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

    def testDrawModes(self):
        self._StartTest('DrawModes')
        cmds.modelEditor('modelPanel4', edit=True, grid=False)

        cmds.move(2, 2, 2, 'persp')
        cmds.rotate(-33, 45, 0, 'persp')
        self.assertSnapshotClose('%s_cross_all_positive.png' % self._testName)

        cmds.move(-2, -2, -2, 'persp')
        cmds.rotate(145, 45, 0, 'persp')
        self.assertSnapshotClose('%s_cross_all_negative.png' % self._testName)

        mayaPathSegment = mayaUtils.createUfePathSegment('|stage|stageShape')
        usdPathSegment = usdUtils.createUfePathSegment('/DrawModes')
        drawModesPath = ufe.Path([mayaPathSegment, usdPathSegment])
        drawModesItem = ufe.Hierarchy.createItem(drawModesPath)
        drawModesPrim = usdUtils.getPrimFromSceneItem(drawModesItem)
        cardGeomAttr = drawModesPrim.GetAttribute('model:cardGeometry')
        cardGeomAttr.Set('box')

        cmds.move(2, 2, 2, 'persp')
        cmds.rotate(-33, 45, 0, 'persp')
        self.assertSnapshotClose('%s_box_all_positive.png' % self._testName)

        cmds.move(-2, -2, -2, 'persp')
        cmds.rotate(145, 45, 0, 'persp')
        self.assertSnapshotClose('%s_box_all_negative.png' % self._testName)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
