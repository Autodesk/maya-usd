#!/usr/bin/env python

#
# Copyright 2026 Sony Interactive Entertainment
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

import mayaUsd.lib

from pxr import UsdGeom

from maya import cmds
from maya import standalone

import fixturesUtils
import testUtils

class MayaUsdStageStatisticsTestCase(unittest.TestCase):
    """Test mayaUsd.lib.ComputeUsdDetails()."""

    SPHERE = {'prims': 1, 'meshes': 1, 'vertices': 382, 'triangles': 760,
              'faces': 400}

    BOTH_SPHERES = {'prims': 3, 'meshes': 2, 'vertices': 764, 'triangles': 1520,
                    'faces': 800}

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

        cls.usdFilePath = testUtils.getTestScene('twoMeshSpheres', 'two_mesh_spheres.usda')
        cls.instancedFilePath = testUtils.getTestScene('instances',
                                                       'instancedTexturedBalls.usda')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def _makeProxyShape(self, filePath=None):
        shapeNode = cmds.createNode('mayaUsdProxyShape')
        cmds.setAttr('{}.filePath'.format(shapeNode),
                     filePath or self.usdFilePath, type='string')
        return shapeNode

    def _subset(self, stats, expected):
        return {key: stats.get(key) for key in expected}

    def testPrimSubtree(self):
        shapeNode = self._makeProxyShape()
        stats = mayaUsd.lib.ComputeUsdDetails(
            objects=['{},/group/Sphere1'.format(shapeNode)])
        self.assertEqual(self._subset(stats, self.SPHERE), self.SPHERE)

    def testOverlappingObjectsCountedOnce(self):
        shapeNode = self._makeProxyShape()
        cmds.select(clear=True)
        wholeStage = self._subset(mayaUsd.lib.ComputeUsdDetails(), self.BOTH_SPHERES)

        # A prim and one of its descendants.
        stats = mayaUsd.lib.ComputeUsdDetails(
            objects=['{},/group'.format(shapeNode),
                     '{},/group/Sphere1'.format(shapeNode)])
        self.assertEqual(self._subset(stats, self.BOTH_SPHERES), wholeStage)

        # The same prim twice.
        stats = mayaUsd.lib.ComputeUsdDetails(
            objects=['{},/group/Sphere1'.format(shapeNode),
                     '{},/group/Sphere1'.format(shapeNode)])
        self.assertEqual(self._subset(stats, self.SPHERE), self.SPHERE)

        # The proxy transform, its shape, and a prim under them.
        transformNode = cmds.listRelatives(shapeNode, parent=True, fullPath=True)[0]
        stats = mayaUsd.lib.ComputeUsdDetails(
            objects=[transformNode, shapeNode, '{},/group/Sphere2'.format(shapeNode)])
        self.assertEqual(self._subset(stats, self.BOTH_SPHERES), wholeStage)

    def testOverlappingSelectionCountedOnce(self):
        shapeNode = self._makeProxyShape()
        cmds.select(clear=True)
        wholeStage = self._subset(mayaUsd.lib.ComputeUsdDetails(), self.BOTH_SPHERES)

        shapePath = cmds.ls(shapeNode, long=True)[0]
        transformNode = cmds.listRelatives(shapeNode, parent=True, fullPath=True)[0]
        cmds.select(transformNode, '{},/group/Sphere1'.format(shapePath))
        stats = mayaUsd.lib.ComputeUsdDetails()
        self.assertEqual(self._subset(stats, self.BOTH_SPHERES), wholeStage)

    def testSiblingObjectsBothCounted(self):
        shapeNode = self._makeProxyShape()
        stats = mayaUsd.lib.ComputeUsdDetails(
            objects=['{},/group/Sphere1'.format(shapeNode),
                     '{},/group/Sphere2'.format(shapeNode)])
        twoSpheres = {key: 2 * value for key, value in self.SPHERE.items()}
        self.assertEqual(self._subset(stats, twoSpheres), twoSpheres)

    def testWholeStage(self):
        self._makeProxyShape()
        cmds.select(clear=True)
        stats = mayaUsd.lib.ComputeUsdDetails()
        self.assertEqual(self._subset(stats, self.BOTH_SPHERES), self.BOTH_SPHERES)

    def testPrimTypeBreakdown(self):
        self._makeProxyShape()
        cmds.select(clear=True)

        types = mayaUsd.lib.ComputeUsdDetails(byType=True)['types']
        self.assertEqual(types['Mesh'], 2)
        self.assertEqual(types['Xform'], 1)

        self.assertEqual(mayaUsd.lib.ComputeUsdDetails(byType=False)['types'], {})

    def testInstanceProxiesChangeWhatIsCounted(self):
        self._makeProxyShape(self.instancedFilePath)
        cmds.select(clear=True)

        drawn = mayaUsd.lib.ComputeUsdDetails()
        self.assertEqual(drawn['prims'], 25)
        self.assertEqual(drawn['instances'], 3)
        self.assertEqual(drawn['instanceProxies'], 21)

        flat = mayaUsd.lib.ComputeUsdDetails(instanceProxies=False)
        self.assertEqual(flat['prims'], 4)
        self.assertEqual(flat['instances'], 3)

    def testUndrawnPrimAxesAreIndependent(self):
        shapeNode = self._makeProxyShape()
        cmds.select(clear=True)

        stage = mayaUsd.lib.GetPrim(shapeNode).GetStage()

        stage.SetEditTarget(stage.GetSessionLayer())
        stage.DefinePrim('/group/Off', 'Xform').SetActive(False)
        stage.CreateClassPrim('/Klass')
        stage.OverridePrim('/Over')

        base = mayaUsd.lib.ComputeUsdDetails()['prims']
        self.assertEqual(base, self.BOTH_SPHERES['prims'])

        for argument in ('includeInactive', 'includeClasses', 'includeOvers'):
            self.assertEqual(
                mayaUsd.lib.ComputeUsdDetails(**{argument: True})['prims'],
                base + 1,
                '{} let in the wrong number of prims'.format(argument))

        self.assertEqual(
            mayaUsd.lib.ComputeUsdDetails(includeInactive=True,
                                          includeClasses=True,
                                          includeOvers=True)['prims'],
            base + 3)

    def testCountsAtShapeTime(self):
        shapeNode = self._makeProxyShape()
        cmds.select(clear=True)

        stage = mayaUsd.lib.GetPrim(shapeNode).GetStage()
        stage.SetEditTarget(stage.GetSessionLayer())
        visibility = UsdGeom.Imageable(stage.GetPrimAtPath('/group/Sphere1')).GetVisibilityAttr()
        visibility.Set(UsdGeom.Tokens.inherited, 1)
        visibility.Set(UsdGeom.Tokens.invisible, 10)

        cmds.setAttr('{}.time'.format(shapeNode), 1)
        self.assertEqual(mayaUsd.lib.ComputeUsdDetails()['meshes'], 2)

        cmds.setAttr('{}.time'.format(shapeNode), 10)
        self.assertEqual(mayaUsd.lib.ComputeUsdDetails()['meshes'], 1)

    def testNoStages(self):
        stats = mayaUsd.lib.ComputeUsdDetails()
        self.assertEqual(stats['prims'], 0)
        self.assertEqual(stats['vertices'], 0)

if __name__ == '__main__':
    unittest.main(verbosity=2)