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
from pxr import Usd, UsdSkel

from maya import cmds
from maya import standalone

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

        # Test blend shape weights at different time codes
        cmds.currentTime(0)
        self.assertEqual(sorted(cmds.getAttr("b1_Deformer.weight")[0]), [-1.0])
        cmds.currentTime(3)
        self.assertEqual(sorted(cmds.getAttr("b1_Deformer.weight")[0]), [-0.15625])
        cmds.currentTime(5)
        self.assertEqual(sorted(cmds.getAttr("b1_Deformer.weight")[0]), [0.15625])
        cmds.currentTime(8)
        self.assertEqual(sorted(cmds.getAttr("b1_Deformer.weight")[0]), [1.0])

        # Expected deltas from the USD file:
        expectedDeltas = {
            0: (0, 0, 0),
            1: (0, 0, 0),
            2: (0, 0, 0),
            3: (0, 0, 0),
            4: (-5, -5, 0),
            5: (5, 5, 0),   # Note: USD has this at index 5
            6: (5, -5, 0),   # Note: USD has this at index 6
            7: (-5, 5, 0)    # Note: USD has this at index 7
        }
        
        # Find the target group and item - weight 1.0 maps to item 6000
        targetGroupIndex = 0
        targetItemIndex = 6000
        
        # The deltas are stored as MFnPointArrayData in inputPointsTarget
        pointsTargetAttr = f"b1_Deformer.inputTarget[0].inputTargetGroup[{targetGroupIndex}].inputTargetItem[{targetItemIndex}].inputPointsTarget"
        componentsTargetAttr = f"b1_Deformer.inputTarget[0].inputTargetGroup[{targetGroupIndex}].inputTargetItem[{targetItemIndex}].inputComponentsTarget"
        
        # Get the component indices (which vertices are affected)
        self.assertTrue(cmds.objExists(componentsTargetAttr), "inputComponentsTarget attribute should exist")
        
        # Get the points (deltas) - this returns a flat list of coordinates
        self.assertTrue(cmds.objExists(pointsTargetAttr), "inputPointsTarget attribute should exist")

        # pointsData is a list of tuples: [(x1, y1, z1, w1), (x2, y2, z2, w2), ...]
        pointsData = cmds.getAttr(pointsTargetAttr)

        self.assertEqual(len(pointsData), 8, f"Expected 8 deltas, but got {len(pointsData)}")

        # The sorted indices from the USD file (after sorting by the C++ code)
        sortedIndices = [0, 1, 2, 3, 4, 5, 6, 7]

        # Now match indices with deltas
        actualDeltas = {}
        foundNonZeroVertices = set()

        for i, vertexIndex in enumerate(sortedIndices):
            if i < len(pointsData):
                deltaX = pointsData[i][0]
                deltaY = pointsData[i][1]
                deltaZ = pointsData[i][2]
                
                actualDeltas[vertexIndex] = (deltaX, deltaY, deltaZ)
                
                # Check if this is a non-zero delta
                if abs(deltaX) > 0.0001 or abs(deltaY) > 0.0001 or abs(deltaZ) > 0.0001:
                    foundNonZeroVertices.add(vertexIndex)
                
                # Check against expected delta
                if vertexIndex in expectedDeltas:
                    expectedDelta = expectedDeltas[vertexIndex]
                    self.assertAlmostEqual(deltaX, expectedDelta[0], places=4,
                                           msg=f"Vertex {vertexIndex} delta X mismatch: expected {expectedDelta[0]}, got {deltaX}")
                    self.assertAlmostEqual(deltaY, expectedDelta[1], places=4,
                                           msg=f"Vertex {vertexIndex} delta Y mismatch: expected {expectedDelta[1]}, got {deltaY}")
                    self.assertAlmostEqual(deltaZ, expectedDelta[2], places=4,
                                           msg=f"Vertex {vertexIndex} delta Z mismatch: expected {expectedDelta[2]}, got {deltaZ}")
        
        # Verify that the non-zero deltas are present (vertices 4, 5, 6, 7 should have deltas)
        nonZeroDeltaVertices = {4, 5, 6, 7}
        self.assertEqual(foundNonZeroVertices, nonZeroDeltaVertices,
                         f"Non-zero delta vertices don't match expected set. Found: {foundNonZeroVertices}, Expected: {nonZeroDeltaVertices}. All deltas found: {actualDeltas}")


if __name__ == '__main__':
    unittest.main(verbosity=2)
