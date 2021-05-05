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


class testVP2RenderDelegateBasisCurves(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using basis curves
    """

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        # cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegateBasisCurvesTest', 'baseline')

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
        testFile = testUtils.getTestScene("basisCurves", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

    def _RunTest(self, complexity, selection):
        globalSelection = ufe.GlobalSelection.get()
        cmds.setAttr('stageShape.cplx', 1)
        self.assertSnapshotClose('%s_unselected_complexity%d.png' % (self._testName, complexity))
        globalSelection.replaceWith(selection)
        self.assertSnapshotClose('%s_selected_complexity%d.png' % (self._testName, complexity))
        globalSelection.clear()

    def _GetSceneItem(self, mayaPathString, usdPathString):
        mayaPathSegment = mayaUtils.createUfePathSegment(mayaPathString)
        usdPathSegment = usdUtils.createUfePathSegment(usdPathString)
        ufePath = ufe.Path([mayaPathSegment, usdPathSegment])
        ufeItem = ufe.Hierarchy.createItem(ufePath)
        return ufeItem

    def testBasisCurves(self):
        self._StartTest('basisCurves')

        cmds.move(20, -10, 10, 'persp')
        cmds.rotate(60, 0, 30, 'persp')
        cmds.modelEditor('modelPanel4', edit=True, grid=False)

        selection = ufe.Selection()
        selection.append(self._GetSceneItem('|stage|stageShape', '/Linear/Tubes/VaryingWidth'))
        selection.append(self._GetSceneItem('|stage|stageShape', '/Linear/Ribbons/VaryingWidth'))
        selection.append(self._GetSceneItem('|stage|stageShape', '/Cubic/Tubes/VaryingWidth'))
        selection.append(self._GetSceneItem('|stage|stageShape', '/Cubic/Ribbons/VaryingWidth'))
        
        self._RunTest(0, selection)
        self._RunTest(1, selection)
        self._RunTest(2, selection)
        self._RunTest(3, selection)


    def testExportedNURBS(self):
        self._StartTest('nurbsCircleExport')

        cmds.move(6, -6, 6, 'persp')
        cmds.rotate(60, 0, 45, 'persp')
        cmds.modelEditor('modelPanel4', edit=True, grid=False)

        selection = ufe.Selection()
        selection.append(self._GetSceneItem('|stage|stageShape', '/nurbsCircle1'))

        self._RunTest(0, selection)
        self._RunTest(1, selection)
        self._RunTest(2, selection)
        self._RunTest(3, selection)



if __name__ == '__main__':
    fixturesUtils.runTests(globals())
