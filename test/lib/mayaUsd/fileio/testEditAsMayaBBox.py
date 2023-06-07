#!/usr/bin/env python

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


import mayaUsd.lib

import ufe

from pxr import Usd, UsdGeom, Gf, Sdf

from maya import cmds
from maya import standalone

import fixturesUtils
import mayaUtils
import testUtils

import unittest

class EditAsMayaBBoxTestCase(unittest.TestCase):

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testGroupBoundingBoxAfterMayaMove(self):
        '''
        Verify that the bounding box of a USD group containing both USD data
        and edited-as-Maya data contains the bounding box of both.
        '''
        
        usdaFile = testUtils.getTestScene('twoMeshSpheres', 'two_mesh_spheres.usda')
        proxyShapeDagPath, usdStage = mayaUtils.createProxyFromFile(usdaFile)
        groupUfePathStr = proxyShapeDagPath + ',/group'
        sphere1UfePathStr = groupUfePathStr + '/Sphere1'
        sphere2UfePathStr = groupUfePathStr + '/Sphere2'

        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.canEditAsMaya(sphere2UfePathStr))
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(sphere2UfePathStr))

        cmds.select('Sphere2', r=True)
        cmds.move(10., 10., 10., relative=True)

        sphere1UfePath = ufe.PathString.path(sphere1UfePathStr)
        sphere1UfeItem = ufe.Hierarchy.createItem(sphere1UfePath)
        sphere1UfeObject3d = ufe.Object3d.object3d(sphere1UfeItem)
        sphere1UfeBBox = sphere1UfeObject3d.boundingBox()

        testUtils.assertVectorAlmostEqual(self, sphere1UfeBBox.min.vector, [-1, -1, -1], 5)
        testUtils.assertVectorAlmostEqual(self, sphere1UfeBBox.max.vector, [1, 1, 1], 5)

        sphere2UfePath = ufe.PathString.path(sphere2UfePathStr)
        sphere2UfeItem = ufe.Hierarchy.createItem(sphere2UfePath)
        sphere2UfeObject3d = ufe.Object3d.object3d(sphere2UfeItem)
        sphere2UfeBBox = sphere2UfeObject3d.boundingBox()

        testUtils.assertVectorAlmostEqual(self, sphere2UfeBBox.min.vector, [-1, -1, -1], 5)
        testUtils.assertVectorAlmostEqual(self, sphere2UfeBBox.max.vector, [1, 1, 1], 5)

        groupUfePath = ufe.PathString.path(groupUfePathStr)
        groupUfeItem = ufe.Hierarchy.createItem(groupUfePath)
        groupUfeObject3d = ufe.Object3d.object3d(groupUfeItem)
        groupUfeBBox = groupUfeObject3d.boundingBox()

        testUtils.assertVectorAlmostEqual(self, groupUfeBBox.min.vector, [4, -1, -1], 5)
        testUtils.assertVectorAlmostEqual(self, groupUfeBBox.max.vector, [6, 11, 11], 5)


if __name__ == '__main__':
    unittest.main(verbosity=2)
