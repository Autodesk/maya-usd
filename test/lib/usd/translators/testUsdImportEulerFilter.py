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

from pxr import Usd

import fixturesUtils

def write_usd_source_file(out_file):
    stage = Usd.Stage.CreateNew(out_file)


def build_skel_scene(out_file):
    cmds.file(f=1, new=1)
    cmds.createNode('transform', name='group')
    cmds.joint()
    cmds.joint(p=(0.0, 2.154, 0.0,))
    
    cmds.select(cl=1)
    cmds.polyPlane(sh=1, sw=1)
    cmds.parent('pPlane1', 'group')
    cmds.skinCluster('joint1', 'pPlane1')

    cmds.setKeyframe('joint1.rx', 'joint1.rz', v=0.0, t=[1])
    cmds.setKeyframe('joint1.rx', 'joint1.rz', v=180.0, t=[2])

    cmds.setKeyframe('joint1.ry', v=-85.968, t=[1])
    cmds.setKeyframe('joint1.ry', v=-85.127, t=[2])
    
    cmds.mayaUSDExport(file=out_file, skl='auto', skn='auto', fr=[1,2])
    
    

def build_xform_scene(out_file):
    cmds.file(f=1, new=1)
    cmds.polyCube()

    cmds.setKeyframe('pCube1.rx', 'pCube1.rz', v=0.0, t=[1])
    cmds.setKeyframe('pCube1.rx', 'pCube1.rz', v=180.0, t=[2])

    cmds.setKeyframe('pCube1.ry', v=-85.968, t=[1])
    cmds.setKeyframe('pCube1.ry', v=-85.127, t=[2])

    cmds.mayaUSDExport(file=out_file, fr=[1,2])


class testUsdImportEulerFilter(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        tempTestPath = os.environ.get('MAYA_APP_DIR')
        cls.skel_file = os.path.join(tempTestPath, "UsdImportEulerFilterTest", "UsdImportEulerFilterTest_skel.usda")
        
        build_skel_scene(cls.skel_file)

        cls.xform_file = os.path.join(tempTestPath, "UsdImportEulerFilterTest", "UsdImportEulerFilterTest_xform.usda")
        build_xform_scene(cls.xform_file)

    def test_skel_rotation_fail(self):
        """The original behaviour should not correct euler angles on import"""
        cmds.file(f=1, new=1)
        cmds.mayaUSDImport(file=self.skel_file, ani=1)

        values =  cmds.keyframe('joint1.rx', q=1, vc=1)
        self.assertNotAlmostEqual(0.0, values[-1])

    def test_xform_rotation_fail(self):
        """The original behaviour should not correct euler angles on import"""
        cmds.file(f=1, new=1)
        cmds.mayaUSDImport(file=self.xform_file, ani=1)

        values =  cmds.keyframe('pCube1.rx', q=1, vc=1)
        self.assertNotAlmostEqual(0.0, values[-1])


    def test_skel_rotation(self):
        """Test that the channels on the imported joints have been euler filtered"""
        cmds.file(f=1, new=1)
        cmds.mayaUSDImport(file=self.skel_file, ani=1, aef=1)

        values =  cmds.keyframe('joint1.rx', q=1, vc=1)
        self.assertAlmostEqual(0.0, values[-1])

    def test_xform_rotation(self):
        """Test that the channels on the imported transforms have been euler filtered"""
        cmds.file(f=1, new=1)
        cmds.mayaUSDImport(file=self.xform_file, ani=1, aef=1)

        values =  cmds.keyframe('pCube1.rx', q=1, vc=1)
        self.assertAlmostEqual(0.0, values[-1])

if __name__ == '__main__':
    unittest.main(verbosity=2)

