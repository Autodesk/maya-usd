#!/usr/bin/env mayapy
#
# Copyright 2023 Apple
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
import tempfile
import unittest

import mayaUsd.lib as mayaUsdLib
import mayaUsd.ufe as mayaUsdUfe

from pxr import Plug, Usd, Sdf, Tf, UsdSkel, UsdGeom

from maya import cmds
from maya.api import OpenMaya
from maya import standalone

import fixturesUtils


def do_setup():
    """Setup the scene"""
    cmds.file(f=1, new=1)
    
    grp = cmds.createNode('transform', name='root')
    pcube = cmds.polyCube()
    cmds.parent(pcube[0], grp)
    cmds.select( d=True )
    
    cmds.select(grp)
    joint_a = cmds.joint()
    joint_a = cmds.joint( p=(0, 0, 0))
    cmds.joint( p=(0, 4, 0))
    
    cmds.skinCluster(joint_a, pcube[0], tsb=1, mi=1)

    cmds.setKeyframe("joint2.sx", t=0.0, v=0.0)
    cmds.setKeyframe("joint2.sx", t=100.0, v=1.0)
    cmds.keyTangent("joint2.sx", wt=1)
    cmds.keyTangent("joint2.sx", e=1, index=(0,), itt='linear', ott='linear')
    cmds.keyTangent("joint2.sx", e=1, index=(1,), itt='auto', ott='auto')

    psphere = cmds.polySphere()
    cmds.setKeyframe(psphere[0]+".sx", t=0.0, v=0.0)
    cmds.setKeyframe(psphere[0]+".sx", t=100.0, v=1.0)
    cmds.keyTangent(psphere[0]+".sx", wt=1)
    cmds.keyTangent(psphere[0]+".sx", e=1, index=(0,), itt='linear', ott='linear')
    cmds.keyTangent(psphere[0]+".sx", e=1, index=(1,), itt='auto', ott='auto')




class TestSparseSampler(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.input_path = fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')
        

        do_setup()

        cls.usd_file_sparse = os.path.join(cls.input_path, 'sparse_samples.usda')
        export_kwargs = export_kwargs = {
                "file": cls.usd_file_sparse,
                "selection" : False,
                "exportDisplayColor": True,
                "materialsScopeName": "Looks",
                "mergeTransformAndShape": True,
                "shadingMode": "useRegistry",
                "defaultMeshScheme": "none",
                "exportSkin": "auto",
                "exportSkels": "auto",
                "frameRange": [70, 99]
            }
        
        cmds.mayaUSDExport(**export_kwargs)
        cls.sparse_stage = Usd.Stage.Open(cls.usd_file_sparse)
    

        cls.usd_file_not_sparse = os.path.join(cls.input_path, 'no_sparse_samples.usda')
        export_kwargs['file'] = cls.usd_file_not_sparse
        export_kwargs['disableSparseSamples'] = True

        cmds.mayaUSDExport(**export_kwargs)
        cls.not_sparse_stage = Usd.Stage.Open(cls.usd_file_not_sparse)


    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    # this test disabled as it will fail.
    def _test_with_sparse_samples_joint(self):
        """Check for duplicate values in the sample data in joint_attr[1]

        This test fails, indicating that the SparseValueWriter has written 2 samples at the same value which causes jitter in the
        output animation. This test should be re-enabled if SparseValueWriter is fixed.

        """
        skel_anim = UsdSkel.Animation.Get(self.sparse_stage, '/root/joint1/Animation')

        scales_attr = skel_anim.GetScalesAttr()
        samples = scales_attr.GetTimeSamples()
        for i in range(len(samples)-1):
            prev = scales_attr.Get(samples[i])
            nxt = scales_attr.Get(samples[i+1])
            # this indicates duplicate values
            self.assertNotAlmostEqual(prev[1][0], nxt[1][0])

    def test_with_sparse_samples_xform(self):
        """Check the same animation values on an xform"""
        prim = self.sparse_stage.GetPrimAtPath('/pSphere1')
        xform = UsdGeom.Xformable(prim)
        scale_op = [x for x in xform.GetOrderedXformOps() if x.GetOpType() == UsdGeom.XformOp.TypeScale][0]
        samples = scale_op.GetTimeSamples()
        for i in range(len(samples)-1):
            prev = scale_op.Get(samples[i])
            nxt = scale_op.Get(samples[i+1])
            self.assertNotAlmostEqual(prev[0], nxt[0])

    def test_without_sparse_samples_joint(self):
        """Check for duplicate values in the sample data in joint_attr[1]"""
        
        skel_anim = UsdSkel.Animation.Get(self.not_sparse_stage, '/root/joint1/Animation')

        scales_attr = skel_anim.GetScalesAttr()
        samples = scales_attr.GetTimeSamples()
        for i in range(len(samples)-1):
            prev = scales_attr.Get(samples[i])
            nxt = scales_attr.Get(samples[i+1])
            self.assertNotAlmostEqual(prev[1][0], nxt[1][0])

    def test_without_sparse_samples_xform(self):
        """Check the same animation values on an xform"""
        prim = self.not_sparse_stage.GetPrimAtPath('/pSphere1')
        xform = UsdGeom.Xformable(prim)
        scale_op = [x for x in xform.GetOrderedXformOps() if x.GetOpType() == UsdGeom.XformOp.TypeScale][0]
        samples = scale_op.GetTimeSamples()
        for i in range(len(samples)-1):
            prev = scale_op.Get(samples[i])
            nxt = scale_op.Get(samples[i+1])
            self.assertNotAlmostEqual(prev[0], nxt[0])

if __name__ == '__main__':
    unittest.main(verbosity=2)

