#!/usr/bin/env mayapy
#
# Copyright 2016 Pixar
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

class testUsdExportVisibilityDefault(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        mayaFile = os.path.join(inputPath, "UsdExportVisibilityDefaultTest", "UsdExportVisibilityDefaultTest.ma")
        cmds.file(mayaFile, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testCurrentFrameVisibilityAsDefault(self):
        # Makes sure that the current frame's visibility is exported as default.
        for frame in (1, 2):
            cmds.currentTime(frame)
            cmds.usdExport(file=os.path.abspath("out_%d.usda" % frame),
                    exportVisibility=True)

        out_1 = Usd.Stage.Open('out_1.usda')
        self.assertEqual(UsdGeom.Imageable.Get(out_1, '/group').ComputeVisibility(), UsdGeom.Tokens.invisible)
        self.assertEqual(UsdGeom.Imageable.Get(out_1, '/group_inverse').ComputeVisibility(), UsdGeom.Tokens.inherited)

        out_2 = Usd.Stage.Open('out_2.usda')
        self.assertEqual(UsdGeom.Imageable.Get(out_2, '/group').ComputeVisibility(), UsdGeom.Tokens.inherited)
        self.assertEqual(UsdGeom.Imageable.Get(out_2, '/group_inverse').ComputeVisibility(), UsdGeom.Tokens.invisible)

if __name__ == '__main__':
    unittest.main(verbosity=2)

