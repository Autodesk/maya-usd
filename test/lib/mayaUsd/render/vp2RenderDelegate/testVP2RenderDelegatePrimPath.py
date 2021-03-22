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


class testVP2RenderDelegatePrimPath(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using a primPath.
    """

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Y-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='y')

        inputPath = fixturesUtils.setUpClass(__file__, initializeStandalone=False, loadPlugin=False)
        cls._baselineDir = os.path.join(inputPath,'VP2RenderDelegatePrimPathTest', 'baseline')
        cls._testDir = os.path.abspath('.')

    def assertSnapshotClose(self, imageName):
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=960, height=540)
        return self.assertImagesClose(baselineImage, snapshotImage)

    @staticmethod
    def _GetUfePath(name):
        mayaSegment = mayaUtils.createUfePathSegment('|stage1|stageShape1')
        usdSegmentString = mayaUsdUfe.usdPathToUfePathSegment(name, -1)
        usdSegment = usdUtils.createUfePathSegment(usdSegmentString)
        ufePath = ufe.Path([mayaSegment, usdSegment])
        return ufePath

    @staticmethod
    def _GetSceneItem(name):
        ufePath = testVP2RenderDelegatePrimPath._GetUfePath(name)
        ufeItem = ufe.Hierarchy.createItem(ufePath)
        return ufeItem

    def testPrimPath(self):

        def testSinglePrim(primPath, imageName):
            cmds.setAttr( 'stageShape1.primPath', primPath, type="string")
            self.assertSnapshotClose('%s.png' % imageName)

            globalSelection = ufe.GlobalSelection.get()
            sceneItem = testVP2RenderDelegatePrimPath._GetSceneItem(primPath)
            globalSelection.append(sceneItem)

            cmds.move(-5,0,0, relative=True)
            self.assertSnapshotClose('%s_moved.png' % imageName)

        mayaUtils.openPrimPathScene()

        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertSnapshotClose('initial.png')

        testSinglePrim("/Cube1", "Cube1")
        testSinglePrim("/Cube1/Cube2", "Cube2")
        testSinglePrim("/Cube1/Cube2/Cube3", "Cube3")

        cmds.setAttr( 'stageShape1.primPath', "", type="string")
        self.assertSnapshotClose('final.png')

        # Test with a invalid prime path
        # Nothing should change in the viewport
        cmds.setAttr( 'stageShape1.primPath', "/invalidPrim", type="string")
        self.assertSnapshotClose('final.png')


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
