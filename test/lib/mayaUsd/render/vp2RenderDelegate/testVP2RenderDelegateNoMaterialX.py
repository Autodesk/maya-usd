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
import unittest


class testVP2RenderDelegateNoMaterialX(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate but with MaterialX
    actively disabled via env var MAYAUSD_VP2_USE_ONLY_PREVIEWSURFACE.
    """

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegateMaterialXTest', 'baseline')

        cls._testDir = os.path.abspath('.')

    def assertSnapshotClose(self, imageName, w=960, h=540):
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=w, height=h)
        return self.assertImagesClose(baselineImage, snapshotImage)

    def _StartTest(self, testName, textured=True):
        mayaUtils.loadPlugin("mayaUsdPlugin")
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, displayTextures=textured)

        self._testName = testName
        testFile = testUtils.getTestScene("MaterialX", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertSnapshotClose('%s_render.png' % self._testName)

    def testWithDisabledMaterialX(self):
        """Make sure the MAYAUSD_VP2_USE_ONLY_PREVIEWSURFACE env var has an effect."""
        cmds.file(force=True, new=True)

        light = cmds.directionalLight(rgb=(1, 1, 1))
        transform = cmds.listRelatives(light, parent=True)[0]
        cmds.xform(transform, ro=(-30, -6, -75), ws=True)
        cmds.setAttr(light+".intensity", 10)
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, lights=False, displayLights="all", displayTextures=True)

        # Pyramid is red under preview surface, green as MaterialX, and
        # blue as display colors. We want red here.
        self._StartTest('ShadedPyramid')


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
