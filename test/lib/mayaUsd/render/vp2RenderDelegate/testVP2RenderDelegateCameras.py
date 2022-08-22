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
import ufeUtils

from mayaUsd import ufe as mayaUsdUfe

from maya import cmds

import ufe
import os

class testVP2RenderDelegateCameras(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using cameras
    """

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegateCamerasTest', 'baseline')

        cls._testDir = os.path.abspath('.')

    def assertSnapshotClose(self, imageName):
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=960, height=540)
        return self.assertImagesClose(baselineImage, snapshotImage)

    def _StartTest(self, testName):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        mayaUtils.loadPlugin("drawUfe")
        self._testName = testName
        testFile = testUtils.getTestScene("camera", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)

    def _RunTest(self):
        cmds.setAttr('stageShape.cplx', 1)

        # reparent one of the cameras to verify that the proxy camera handles this properly
        # !this check is temporarily disabled because of a bug in PathSubject::addRemoveAllCallbackHelper 
        #cmds.parent('|stage|stageShape,/cameras/cam1', '|stage|stageShape,/cameras2')

        # unselect all and compare images
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertSnapshotClose('%s_unselected.png' % (self._testName))

        # select all 2 cameras and compare images
        selection = ufe.Selection()
        selection.append(self._GetSceneItem('|stage|stageShape', '/cameras/cam1'))
        selection.append(self._GetSceneItem('|stage|stageShape', '/cameras/cam2'))
        globalSelection.replaceWith(selection)
        self.assertSnapshotClose('%s_selected.png' % (self._testName))

        # change attributes that affect the camera image
        ufeItem = ufeUtils.createItem('|stage|stageShape,/cameras/cam2')
        cameraPrim = usdUtils.getPrimFromSceneItem(ufeItem)
        cameraPrim.GetAttribute('horizontalAperture').Set(100000)
        cameraPrim.GetAttribute('verticalAperture').Set(100000)
        cameraPrim.GetAttribute('focalLength').Set(100000)

        # move all 2 cameras and compare images
        cmds.move(-2, -2, -2, relative=True)
        self.assertSnapshotClose('%s_moved.png' % (self._testName))

        # rename the stage to verify that the proxy camera handles this properly
        cmds.rename('|stage', 'stage2')

        # switch color management view transfrom to verify that the proxy camera handles this properly
        cmds.modelEditor('modelPanel4', edit=True, viewTransformName='Log')

        # select the stage and compare images
        globalSelection.clear()
        cmds.select('|stage2')
        self.assertSnapshotClose('%s_stage_selected.png' % (self._testName))

        # delete one of the cameras to verify that the proxy camera handles this properly
        cmds.delete('|stage2|stage2Shape,/cameras/cam2')

        # move the stage and compare images
        cmds.move(4, 4, 4, absolute=True)
        self.assertSnapshotClose('%s_stage_moved.png' % (self._testName))
        
        # add a new USD camera and compare images        
        stage = mayaUsdUfe.getStage("|stage2|stage2Shape")
        stage.DefinePrim('/cameras2/cam3', "Camera")
        cmds.select( clear=True )
        self.assertSnapshotClose('%s_with_new_camera.png' % (self._testName))

    def _GetSceneItem(self, mayaPathString, usdPathString):
        mayaPathSegment = mayaUtils.createUfePathSegment(mayaPathString)
        usdPathSegment = usdUtils.createUfePathSegment(usdPathString)
        ufePath = ufe.Path([mayaPathSegment, usdPathSegment])
        ufeItem = ufe.Hierarchy.createItem(ufePath)
        return ufeItem

    def testCameras(self):
        self._StartTest('RenderCameras')

        cmds.move(-10, 15, 20, 'persp')
        cmds.rotate(-30, -30, 0, 'persp')
        cmds.modelEditor('modelPanel4', edit=True, grid=False)
        cmds.displayColor('camera', 21, active=True)
        
        self._RunTest()



if __name__ == '__main__':
    fixturesUtils.runTests(globals())
