#!/usr/bin/env mayapy
#
# Copyright 2025 Autodesk, Inc.
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

import os
import unittest

from pxr import Usd

from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdExportAppendAnim(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

    def _ExportFrame(self, usdFilePath, frame):
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            exportComponentTags=False,
            exportMaterials=False,
            frameRange=(frame, frame),
            append=bool(frame > 1))


    def _AssertTimeSamples(self, stage, primName, expectedSamples):
        cube = stage.GetPrimAtPath('/' + primName)
        attr = cube.GetAttribute('xformOp:translate')
        timeSamples = attr.GetTimeSamples()
        self.assertEqual(timeSamples, expectedSamples)

    def testAppendAnim(self):
        '''
        Tests exporting a cube with animation, appending to the same file.
        Verify that all animation frames are in the same XformOp, not in
        separate channel XformOps.
        '''
        cube = cmds.polyCube(w=2, h=2, d=2, sx=1, sy=1, sz=1)[0]
        cmds.setKeyframe(cube + ".t")
        cmds.currentTime(20)
        cmds.setAttr(cube + ".translateX", 10)
        cmds.setKeyframe(cube + ".t")

        usdFilePath = os.path.abspath('UsdExportAppendAnimTest.usda')
        for frame in range(1, 5):
            self._ExportFrame(usdFilePath, frame)

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)
    
        self._AssertTimeSamples(stage, cube, [1.0, 2.0, 3.0, 4.0])


if __name__ == '__main__':
    unittest.main(verbosity=2)
