#!/usr/bin/env mayapy
#
# Copyright 2024 Autodesk
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

import unittest, os
from pxr import Gf, Usd, UsdSkel

from maya import cmds
from maya import standalone

import maya.api.OpenMaya as OM

import fixturesUtils


class testUsdImportBlendShapes(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)


    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()


    def test_BlendShapesImport(self):
        cmds.file(new=True, force=True)

        path = os.path.join(self.inputPath, "UsdImportSkeleton", "morpher_box.usd")

        cmds.usdImport(file=path, readAnimData=True, primPath="/root",
                       shadingMode=[["none", "default"], ])

        stage = Usd.Stage.Open(path)
        skelCache = UsdSkel.Cache()
        
        skelRootPrim = stage.GetPrimAtPath("/root")
        self.assertTrue(skelRootPrim.IsA(UsdSkel.Root))

        skelCache.Populate(UsdSkel.Root(skelRootPrim), Usd.PrimDefaultPredicate)

        skel = UsdSkel.Skeleton.Get(stage, "/root/Bones")
        self.assertTrue(skel)

        skelQuery = skelCache.GetSkelQuery(skel)
        self.assertTrue(skelQuery)

        meshPrim = stage.GetPrimAtPath("/root/b1")
        self.assertTrue(meshPrim)

        skinningQuery = skelCache.GetSkinningQuery(meshPrim)
        self.assertTrue(skinningQuery)

        self.assertEqual(cmds.nodeType("b1_Deformer"), "blendShape")

        regularShape = sorted(
                cmds.listConnections(
                    "b1_Deformer.inputTarget[0].inputTargetGroup[0].inputTargetItem[6000].inputGeomTarget", 
                    destination=False, source=True, plugs=True))
        self.assertEqual(regularShape, ['Box0002.worldMesh'])

        inBetween = sorted(
                cmds.listConnections(
                    "b1_Deformer.inputTarget[0].inputTargetGroup[0].inputTargetItem[4000].inputGeomTarget", 
                    destination=False, source=True, plugs=True))
        self.assertEqual(inBetween, ['IBT_1.worldMesh'])

        cmds.currentTime(0)
        self.assertEqual(sorted(cmds.getAttr("b1_Deformer.weight")[0]), [-1.0])
        cmds.currentTime(3)
        self.assertEqual(sorted(cmds.getAttr("b1_Deformer.weight")[0]), [-0.15625])
        cmds.currentTime(5)
        self.assertEqual(sorted(cmds.getAttr("b1_Deformer.weight")[0]), [0.15625])
        cmds.currentTime(8)
        self.assertEqual(sorted(cmds.getAttr("b1_Deformer.weight")[0]), [1.0])


if __name__ == '__main__':
    unittest.main(verbosity=2)
