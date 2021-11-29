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

class primUpdaterTest(mayaUsdLib.PrimUpdater):
    pushCopySpecsCalled = False
    discardEditsCalled = False

    def __init__(self, *args, **kwargs):
        super(primUpdaterTest, self).__init__(*args, **kwargs)

    def pushCopySpecs(self, srcStage, srcLayer, srcSdfPath, dstStage, dstLayer, dstSdfPath):
        self.pushCopySpecsCalled = True
        return True

    def discardEdits(self,context):
        self.discardEditsCalled = True
        return True

class testPrimUpdater(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSimplePrimUpdater(self):
        mayaUsdLib.PrimUpdater.Register(primUpdaterTest, "test", "reference", primUpdaterTest.Supports.Push.value + primUpdaterTest.Supports.Clear.value + primUpdaterTest.Supports.AutoPull.value)

        # Those need to be change when performing a real test.
        self.assertFalse(primUpdaterTest.pushCopySpecsCalled)
        self.assertFalse(primUpdaterTest.discardEditsCalled)


if __name__ == '__main__':
    unittest.main(verbosity=2)
