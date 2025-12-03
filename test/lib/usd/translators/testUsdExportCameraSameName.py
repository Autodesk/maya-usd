#!/usr/bin/env mayapy
#
# Copyright 2025 Autodesk
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
from pxr import UsdGeom

from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdExportCameraWithSameName(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportMultiCameras(self):
        """
        Test that having multiple cameras with the same name and exporting
        by specifying a camera node name works. It used to fail because we
        would confuse both cameras.
        """
        cmds.file(force=True, new=True)

        # create a camera and group it
        cmds.camera()
        cmds.rename('cam')
        cmds.group(name='backup')

        # create a second camera with the same name
        cmds.camera()
        cmds.rename('cam')
        
        # export it to USD
        usdFilePath = os.path.abspath('UsdExportCameraTest.usda')
        if os.path.exists(usdFilePath):
            os.remove(usdFilePath)
        cmds.usdExport("|cam", file=usdFilePath)
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        # Verify the camera in USD
        cam = UsdGeom.Camera.Get(stage, '/cam')
        self.assertTrue(cam)


if __name__ == '__main__':
    unittest.main(verbosity=2)
