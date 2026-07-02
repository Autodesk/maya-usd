#!/usr/bin/env mayapy
#
# Copyright 2026 Pixar
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

"""
Tests that expanded pxrUsdReferenceAssembly sub-proxies have correct
transforms and bounding boxes (no double-transform).
"""

from maya import cmds
from maya import standalone

import os
import unittest


class testUsdReferenceAssemblyExpandedTransforms(unittest.TestCase):

    ASSEMBLY_TYPE_NAME = 'pxrUsdReferenceAssembly'
    PROXY_TYPE_NAME = 'pxrUsdProxyShape'

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _SetupScene(self, usdFilePath, primPath=None, translate=None):
        """Sets up a scene with an assembly node. Returns the node name."""
        cmds.file(new=True, force=True)

        usdFile = os.path.abspath(usdFilePath)
        assemblyNode = cmds.assembly(name='TestAssemblyNode',
            type=self.ASSEMBLY_TYPE_NAME)
        cmds.setAttr('%s.filePath' % assemblyNode, usdFile, type='string')
        if primPath:
            cmds.setAttr('%s.primPath' % assemblyNode, primPath, type='string')
        if translate:
            cmds.setAttr('%s.tx' % assemblyNode, translate[0])
            cmds.setAttr('%s.ty' % assemblyNode, translate[1])
            cmds.setAttr('%s.tz' % assemblyNode, translate[2])

        return assemblyNode

    def _GetNamespace(self, assemblyNode):
        """Returns the namespace used by children of the assembly node."""
        children = cmds.listRelatives(assemblyNode, children=True,
            fullPath=True) or []
        if children:
            shortName = children[0].split('|')[-1]
            if ':' in shortName:
                return shortName.rsplit(':', 1)[0]
        return 'NS_%s' % assemblyNode

    def _GetProxyShapes(self, rootNode):
        """Returns all pxrUsdProxyShape nodes under rootNode."""
        return cmds.listRelatives(rootNode, allDescendents=True,
            fullPath=True, type=self.PROXY_TYPE_NAME) or []

    def _AssertBBoxClose(self, bb1, bb2, tolerance=2.0):
        """Asserts two bounding boxes are close within absolute tolerance."""
        self.assertEqual(len(bb1), 6)
        self.assertEqual(len(bb2), 6)
        for i in range(6):
            diff = abs(bb1[i] - bb2[i])
            self.assertLessEqual(diff, tolerance,
                msg='BBox component %d: %f vs %f (diff %f > tolerance %f)'
                % (i, bb1[i], bb2[i], diff, tolerance))

    def testSubProxiesHaveSkipRootPrimTransform(self):
        """Sub-proxies in Expanded mode have skipRootPrimTransform=True."""
        assemblyNode = self._SetupScene('ComplexSet.usda', '/ComplexSet')
        cmds.assembly(assemblyNode, edit=True, active='Expanded')

        proxyShapes = self._GetProxyShapes(assemblyNode)
        self.assertTrue(len(proxyShapes) > 0)

        for proxyShape in proxyShapes:
            self.assertTrue(
                cmds.attributeQuery('skipRootPrimTransform',
                    node=proxyShape, exists=True),
                '%s missing skipRootPrimTransform' % proxyShape)
            self.assertTrue(
                cmds.getAttr('%s.skipRootPrimTransform' % proxyShape),
                '%s should have skipRootPrimTransform=True' % proxyShape)

    def testExpandedTransformMatchesUsdPrim(self):
        """Maya transform nodes match the USD prim's xformOps."""
        assemblyNode = self._SetupScene('ComplexSet.usda', '/ComplexSet')
        cmds.assembly(assemblyNode, edit=True, active='Expanded')

        # ComplexSet.usda 'Ref' prim has xformOp:translate = (0, 30, 5).
        ns = self._GetNamespace(assemblyNode)
        refTransform = '%s:Ref' % ns
        self.assertTrue(cmds.objExists(refTransform))

        self.assertAlmostEqual(cmds.getAttr('%s.tx' % refTransform), 0.0, places=4)
        self.assertAlmostEqual(cmds.getAttr('%s.ty' % refTransform), 30.0, places=4)
        self.assertAlmostEqual(cmds.getAttr('%s.tz' % refTransform), 5.0, places=4)

    def testExpandedBoundingBoxMatchesCollapsed(self):
        """Expanded bbox approximates Collapsed bbox (no doubled offsets)."""
        assemblyNode = self._SetupScene('ComplexSet.usda', '/ComplexSet',
            translate=(10.0, 20.0, 30.0))

        cmds.assembly(assemblyNode, edit=True, active='Collapsed')
        collapsedBBox = cmds.exactWorldBoundingBox(assemblyNode)

        cmds.assembly(assemblyNode, edit=True, active='Expanded')
        expandedBBox = cmds.exactWorldBoundingBox(assemblyNode)

        # Tolerance accounts for axis-aligned bbox accumulation differences
        # across sub-proxies. A doubled transform would differ by 30+ units.
        self._AssertBBoxClose(collapsedBBox, expandedBBox, tolerance=2.0)

    def testNestedCollapsePointTransforms(self):
        """
        Nested collapse points accumulate ancestor transforms correctly.
        NestedRef(0,-30,-5) > DirectChildRef(0,10,10) => world (0,-20,5).
        """
        assemblyNode = self._SetupScene('ComplexSet.usda', '/ComplexSet')
        cmds.assembly(assemblyNode, edit=True, active='Expanded')

        ns = self._GetNamespace(assemblyNode)

        # Verify NestedRef has its USD translate applied.
        nestedRefTransform = '%s:NestedRef' % ns
        self.assertTrue(cmds.objExists(nestedRefTransform))
        self.assertAlmostEqual(cmds.getAttr('%s.ty' % nestedRefTransform), -30.0, places=4)
        self.assertAlmostEqual(cmds.getAttr('%s.tz' % nestedRefTransform), -5.0, places=4)

        # DirectChildRef world position = parent + own = (0,-20,5).
        directChildRefTransform = '%s:DirectChildRef' % ns
        self.assertTrue(cmds.objExists(directChildRefTransform))

        worldPos = cmds.xform(directChildRefTransform,
            query=True, worldSpace=True, translation=True)
        self.assertAlmostEqual(worldPos[0], 0.0, places=3)
        self.assertAlmostEqual(worldPos[1], -20.0, places=3)
        self.assertAlmostEqual(worldPos[2], 5.0, places=3)


if __name__ == '__main__':
    unittest.main(verbosity=2)
