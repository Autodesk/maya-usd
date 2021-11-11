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

import mayaUsd.lib as mayaUsdLib

from pxr import Usd

from maya import cmds
from maya import standalone

import fixturesUtils, os

import unittest

class exportChaserTest(mayaUsdLib.ExportChaser):
    ExportDefaultCalled = False
    ExportFrameCalled = False
    PostExportCalled = False
    NotCalled = False

    def ExportDefault(self):
        exportChaserTest.ExportDefaultCalled = True
        return self.ExportFrame(Usd.TimeCode.Default())

    def ExportFrame(self, frame):
        exportChaserTest.ExportFrameCalled = True
        return True

    def PostExport(self):
        exportChaserTest.PostExportCalled = True
        return True

class testExportChaser(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSimpleExportChaser(self):
        mayaUsdLib.ExportChaser.Register(exportChaserTest, "test")
        cmds.polySphere(r = 3.5, name='apple')

        usdFilePath = os.path.join(self.temp_dir,'testExportChaser.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            chaser=['test'],
            shadingMode='none')

        self.assertTrue(exportChaserTest.ExportDefaultCalled)
        self.assertTrue(exportChaserTest.ExportFrameCalled)
        self.assertTrue(exportChaserTest.PostExportCalled)
        self.assertFalse(exportChaserTest.NotCalled)


if __name__ == '__main__':
    unittest.main(verbosity=2)
