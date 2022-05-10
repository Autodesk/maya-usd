#!/usr/bin/env mayapy
#
# Copyright 2022 Autodesk
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


class testVP2RenderDelegatePoints(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using points
    """

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegatePointsTest', 'baseline')

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
        testFile = testUtils.getTestScene("points", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

    def _RunTest(self, selection):
        globalSelection = ufe.GlobalSelection.get()
        cmds.setAttr('stageShape.cplx', 1)
        self.assertSnapshotClose('%s_unselected.png' % (self._testName))
        globalSelection.replaceWith(selection)
        self.assertSnapshotClose('%s_selected.png' % (self._testName))
        globalSelection.clear()

    def _GetSceneItem(self, mayaPathString, usdPathString):
        mayaPathSegment = mayaUtils.createUfePathSegment(mayaPathString)
        usdPathSegment = usdUtils.createUfePathSegment(usdPathString)
        ufePath = ufe.Path([mayaPathSegment, usdPathSegment])
        ufeItem = ufe.Hierarchy.createItem(ufePath)
        return ufeItem

    def testPoints(self):
        self._StartTest('points')

        cmds.move(20, -20, 25, 'persp')
        cmds.rotate(60, 0, 30, 'persp')
        cmds.modelEditor('modelPanel4', edit=True, grid=False)

        selection = ufe.Selection()
        selection.append(self._GetSceneItem('|stage|stageShape', '/SimplePoints'))
        selection.append(self._GetSceneItem('|stage|stageShape', '/SimplePoints2'))
        
        self._RunTest(selection)



if __name__ == '__main__':
    fixturesUtils.runTests(globals())
