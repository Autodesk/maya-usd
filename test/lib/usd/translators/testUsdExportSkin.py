#!/usr/bin/env mayapy
#
# Copyright 2023 Apple Inc. All rights reserved.
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

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as OM

from pxr import Gf, Sdf, Tf, Usd, UsdGeom, UsdSkel, UsdUtils, Vt

import fixturesUtils


def build_scene():
    """Set up simple skinCluster, add a polyReduce after"""
    cmds.file(f=1, new=1)
    root = cmds.joint()
    cmds.joint()
    sphere = cmds.polySphere()
    cmds.select(root, add=1)
    group = cmds.group()

    cmds.skinCluster(root, sphere[0])
    cmds.select(sphere[0])

    cmds.polyReduce(p=50)
    return group

class TestUsdExportSkin(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)
        # cls.outFile = os.path.join

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def test_exportSkinDifferingTopology(self):
        """Raise a runtime error if the topology has changed downstream from the skinCluster"""
        grp = build_scene()
        self.assertRaises(RuntimeError, cmds.mayaUSDExport, file="Does_not_export.usdc", skn="auto", skl="auto")


if __name__ == '__main__':
    unittest.main(verbosity=2)
