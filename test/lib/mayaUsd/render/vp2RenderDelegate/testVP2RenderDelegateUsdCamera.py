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
import sys
import os


class testVP2RenderDelegateUsdCamera(imageUtils.ImageDiffingTestCase):
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
            'VP2RenderDelegateUsdCameraTest', 'baseline')

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
        testFile = testUtils.getTestScene("camera", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

    def testAnimatedUsdCamera(self):
        self._StartTest('camera_xformOp_transform_rotateX')
        # at time 1 look through the camera and we should see the cube
        cmds.currentTime(1)
        cmds.lookThru('|stage|stageShape,/cam')
        self.assertSnapshotClose('%s_unselected.png' % self._testName)
        # at time 2 the near clip plane increases and the cube is not drawn
        cmds.currentTime(2)
        self.assertSnapshotClose('%s_insideNearClipPlane.png' % self._testName)
        # at time 3 the near clip plane is restored so the cube is drawn, and the camera moves
        cmds.currentTime(3)
        self.assertSnapshotClose('%s_cameraMoved.png' % self._testName)

    def testStaticUsdCamera(self):
        self._StartTest('TranslateRotate_vs_xform')
        
        mayaPathSegment = mayaUtils.createUfePathSegment('|stage|stageShape')
        print(dir(mayaPathSegment), file=sys.stderr)
        #camera1 doesn't work correctly right now because of an xformOp:transform issue MAYA-110515
        #cam1UsdPathSegment = usdUtils.createUfePathSegment('cameras/cam1/camera1')
        #cmds.lookThru('|stage|stageShape,/cameras/cam1/camera1')
        #self.assertSnapshotClose('%s_cam1_unselected.png' % self._testName)
        
        cmds.lookThru('|stage|stageShape,/cameras/cam2/camera2')
        self.assertSnapshotClose('%s_cam2_unselected.png' % self._testName)

        # create a Ufe transform3d and move cam2 somewhere else and do a snapshot
        cam2UsdPathSegment = usdUtils.createUfePathSegment('/cameras/cam2/camera2')
        cam2Path = ufe.Path([mayaPathSegment, cam2UsdPathSegment])
        cam2Item = ufe.Hierarchy.createItem(cam2Path)
        cam2Transform3d = ufe.Transform3d.transform3d(cam2Item)
        print(dir(cam2Transform3d))
        cam2Transform3d.translate(-1, 0, 0)
        self.assertSnapshotClose('%s_cam2_translated.png' % self._testName)
        
        # create a Ufe Camera and modify the near clip plane and do a snapshot
        # ufe.Camera is not in Ufe 2.0.0 MAYA-110675
        #cam2Camera = ufe.Camera.camera(cam2Item)
        #cam2Camera.nearClipPlane(10)
        #self.assertSnapshotClose('%s_cam2_insideNearClipPlane.png' % self._testName)

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
