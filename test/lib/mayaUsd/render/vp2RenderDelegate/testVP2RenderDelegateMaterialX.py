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


class testVP2RenderDelegateMaterialX(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate of a MaterialX scene.
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

    def _StartTest(self, testName):
        mayaUtils.loadPlugin("mayaUsdPlugin")
        
        self._testName = testName
        testFile = testUtils.getTestScene("MaterialX", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertSnapshotClose('%s_render.png' % self._testName)

    def testUVStreamManagement(self):
        """Test that a scene without primvar readers renders correctly if it
           uses indexed UV streams"""
        cmds.file(force=True, new=True)
        cmds.move(2, -2, 1.5, 'persp')
        self._StartTest('MtlxUVStreamTest')

    def testMayaSurfaces(self):
        cmds.file(force=True, new=True)
        cmds.move(6, -6, 6, 'persp')
        cmds.rotate(60, 0, 45, 'persp')
        self._StartTest('MayaSurfaces')

    def testMayaPlace2dTexture(self):
        mayaUtils.loadPlugin("mayaUsdPlugin")

        # Too much differences between Linux and Windows otherwise
        cmds.setAttr("hardwareRenderingGlobals.multiSampleEnable", True)

        testFile = testUtils.getTestScene("MaterialX", "place2dTextureShowcase.ma")
        cmds.file(testFile, force=True, open=True)
        cmds.move(0, 7, -1.5, 'persp')
        cmds.rotate(-90, 0, 0, 'persp')        
        self.assertSnapshotClose('place2dTextureShowcase_Maya_render.png', 960, 960)
        usdFilePath = os.path.join(self._testDir, "place2dTextureShowcase.usda")
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['MaterialX'],
            materialsScopeName='Materials')
        xform, shape = mayaUtils.createProxyFromFile(usdFilePath)
        cmds.move(0, 0, 1, xform)
        self.assertSnapshotClose('place2dTextureShowcase_USD_render.png', 960, 960)

        cmds.delete(xform)
        xform = cmds.mayaUSDImport(file=usdFilePath, shadingMode=[["useRegistry","MaterialX"]])
        cmds.move(0, 0, 1, xform)
        self.assertSnapshotClose('place2dTextureShowcase_Import_render.png', 960, 960)

        cmds.setAttr("hardwareRenderingGlobals.multiSampleEnable", True)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
