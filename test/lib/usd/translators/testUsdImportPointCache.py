#!/usr/bin/env mayapy
#
# Copyright 2023 Autodesk
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
import os
import unittest
from maya import cmds, standalone


class testUsdImportPointCache(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.input_dir = fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')
        cls.usd_file = os.path.join(cls.input_dir, "UsdImportPointCacheTest/pointCache.usda")

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def getKeyframes(cls, path):
        """Get all keyframe values on a prims attributes"""
        frames = set()
        prim = cls.stage.GetPrimAtPath(path)
        attrs = prim.GetAuthoredAttributes()

        for attr in attrs:
            samples = attr.GetTimeSamples()
            frames.update(samples)

        return sorted(frames)

    def testImportPointCache(self):
        """Test that the point cache is imported correctly."""

        cmds.file(f=True, new=True)

        namespace = 'pointCache'
        cmds.file(self.usd_file, namespace=namespace, mergeNamespacesOnClash=False,
          reference=True, type='USD Import', ignoreVersion=True, sharedReferenceFile=False,
          groupLocator=False, deferReference=False, lockReference=False,
          options="assemblyRep=Import;readAnimData=1;useAsAnimationCache=1;")

        nodeName = 'pointCache:usdPointBasedDeformerNode_group1_pCube1'
        primPath = cmds.getAttr(nodeName + '.primPath')
        self.assertEqual(primPath, '/group1/pCube1')

        nodeName = 'pointCache:pCube1'

        cmds.currentTime(0)
        vtx = cmds.pointPosition(nodeName + '.vtx[1]')
        self.assertAlmostEqual(vtx[0], 0.5)

        cmds.currentTime(1050)
        vtx = cmds.pointPosition(nodeName + '.vtx[1]')
        self.assertAlmostEqual(vtx[0], 13.99111366)

if __name__ == '__main__':
    unittest.main(verbosity=2)
