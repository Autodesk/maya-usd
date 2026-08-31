#!/usr/bin/env python

#
# Copyright 2026 Autodesk
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

import unittest

import mayaUsd.ufe

from maya import cmds
from maya import standalone

import fixturesUtils
import testUtils

class MayaUsdStageStatsCommandTestCase(unittest.TestCase):
    """Test the mayaUsdStageStats command."""

    SPHERE = {'prims': 1, 'meshes': 1, 'vertices': 382, 'triangles': 760, 'faces': 400,
              'normals': 0}

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

        cls.usdFilePath = testUtils.getTestScene('twoMeshSpheres', 'two_mesh_spheres.usda')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def _makeProxyShape(self):
        shapeNode = cmds.createNode('mayaUsdProxyShape')
        cmds.setAttr('{}.filePath'.format(shapeNode), self.usdFilePath, type='string')
        return shapeNode

    def _stats(self, *objects, **flags):
        """mayaUsdStageStats -q -allCounts, parsed into a dict of ints."""
        flags.setdefault('query', True)
        flags.setdefault('allCounts', True)
        tokens = cmds.mayaUsdStageStats(*objects, **flags) or []
        parsed = {}
        for token in tokens:
            key, _, value = token.partition('=')
            parsed[key] = int(value)
        return parsed

    def testPrimSubtreeArgument(self):
        shapeNode = self._makeProxyShape()
        stats = self._stats('{},/group/Sphere1'.format(shapeNode))
        self.assertEqual(stats, self.SPHERE)

    def testIndividualFlagsMatchAllCounts(self):
        self._makeProxyShape()
        cmds.select(clear=True)

        expected = self._stats()
        self.assertEqual(cmds.mayaUsdStageStats(q=True, primCount=True), expected['prims'])
        self.assertEqual(cmds.mayaUsdStageStats(q=True, meshCount=True), expected['meshes'])
        self.assertEqual(cmds.mayaUsdStageStats(q=True, vertexCount=True), expected['vertices'])
        self.assertEqual(
            cmds.mayaUsdStageStats(q=True, triangleCount=True), expected['triangles'])
        self.assertEqual(cmds.mayaUsdStageStats(q=True, faceCount=True), expected['faces'])
        self.assertEqual(cmds.mayaUsdStageStats(q=True, normalCount=True), expected['normals'])

if __name__ == '__main__':
    unittest.main(verbosity=2)