#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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

from ufeTestUtils import mayaUtils, usdUtils
from ufeTestUtils.testUtils import assertVectorAlmostEqual

import ufe
import mayaUsd.ufe
import maya.cmds as cmds

import unittest
import os

def childrenNames(children):
    return [str(child.path().back()) for child in children]

def matrixToList(m):
    mList = []
    for i in m.matrix:
        mList = mList + i
    return mList

class OpenFileCtx(object):
    def __init__(self, fileName):
        self._fileName = fileName

    def __enter__(self):
        filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "test-samples", "parentCmd", self._fileName)
        cmds.file(filePath, force=True, open=True)

    def __exit__(self, type, value, traceback):
        # Close the file.
        cmds.file(force=True, new=True)

class ParentCmdTestCase(unittest.TestCase):
    '''Verify the Maya parent command on a USD scene.'''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    @classmethod
    def tearDownClass(cls):
        cmds.file(new=True, force=True)

    def setUp(self):
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # Load a file that has the same scene in both the Maya Dag
        # hierarchy and the USD hierarchy.
        filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "test-samples", "parentCmd", "simpleSceneMayaPlusUSD_TRS.ma" )
        cmds.file(filePath, force=True, open=True)

        # Clear selection to start off
        cmds.select(clear=True)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2013', 'testParentRelative only available in UFE preview version 0.2.13 and greater')
    def testParentRelative(self):
        # Create scene items for the cube and the cylinder.
        shapeSegment = mayaUtils.createUfePathSegment(
            "|world|mayaUsdProxy1|mayaUsdProxyShape1")
        cubePath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/pCube1")])
        cubeItem = ufe.Hierarchy.createItem(cubePath)
        cylinderPath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/pCylinder1")])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(shapeSegment))

        # check GetLayerStack behavior
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        # The cube is not a child of the cylinder.
        cylHier = ufe.Hierarchy.hierarchy(cylinderItem)
        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 0)

        # Get the components of the cube's local transform.
        cubeT3d = ufe.Transform3d.transform3d(cubeItem)
        cubeT = cubeT3d.translation()
        cubeR = cubeT3d.rotation()
        cubeS = cubeT3d.scale()

        # Parent cube to cylinder in relative mode, using UFE path strings.
        cmds.parent("|mayaUsdProxy1|mayaUsdProxyShape1,/pCube1",
                    "|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1",
                    relative=True)

        # Confirm that the cube is now a child of the cylinder.
        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 1)
        self.assertIn("pCube1", childrenNames(cylChildren))

        # Undo: the cube is no longer a child of the cylinder.
        cmds.undo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 0)

        # Redo: confirm that the cube is again a child of the cylinder.
        cmds.redo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 1)
        self.assertIn("pCube1", childrenNames(cylChildren))

        # Confirm that the cube's local transform has not changed.  Must
        # re-create the item, as its path has changed.
        cubeChildPath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/pCylinder1/pCube1")])
        cubeChildItem = ufe.Hierarchy.createItem(cubeChildPath)
        cubeChildT3d = ufe.Transform3d.transform3d(cubeChildItem)

        self.assertEqual(cubeChildT3d.translation(), cubeT)
        self.assertEqual(cubeChildT3d.rotation(),    cubeR)
        self.assertEqual(cubeChildT3d.scale(),       cubeS)

        # Move the parent
        ufe.GlobalSelection.get().append(cylinderItem)

        cmds.move(0, 10, 0, relative=True)

        # Do the same on the equivalent Maya objects.
        cmds.parent("pCube1", "pCylinder1", relative=True)
        cmds.move(0, 10, 0, "pCylinder1", relative=True)

        # Get its world space transform.
        cubeWorld = cubeChildT3d.inclusiveMatrix()
        cubeWorldList = matrixToList(cubeWorld)

        # Do the same for the equivalent Maya object.
        mayaCubeWorld = cmds.xform(
            "|pCylinder1|pCube1", q=True, ws=True, m=True)

        # Equivalent Maya and USD objects must have the same world transform. 
        assertVectorAlmostEqual(self, cubeWorldList, mayaCubeWorld)

        # Undo to bring scene to its original state.
        for i in range(4):
            cmds.undo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 0)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2013', 'testParentAbsolute only available in UFE preview version 0.2.13 and greater')
    def testParentAbsolute(self):
        # Create scene items for the cube and the cylinder.
        shapeSegment = mayaUtils.createUfePathSegment(
            "|world|mayaUsdProxy1|mayaUsdProxyShape1")
        cubePath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/pCube1")])
        cubeItem = ufe.Hierarchy.createItem(cubePath)
        cylinderPath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/pCylinder1")])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(shapeSegment))

        # check GetLayerStack behavior
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        # The cube is not a child of the cylinder.
        cylHier = ufe.Hierarchy.hierarchy(cylinderItem)
        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 0)

        # Get its world space transform.
        cubeT3d = ufe.Transform3d.transform3d(cubeItem)
        cubeWorld = cubeT3d.inclusiveMatrix()
        cubeWorldListPre = matrixToList(cubeWorld)

        # Parent cube to cylinder in absolute mode (default), using UFE
        # path strings.
        cmds.parent("|mayaUsdProxy1|mayaUsdProxyShape1,/pCube1",
                    "|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1")

        # Confirm that the cube is now a child of the cylinder.
        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 1)
        self.assertIn("pCube1", childrenNames(cylChildren))

        # Undo: the cube is no longer a child of the cylinder.
        cmds.undo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 0)

        # Redo: confirm that the cube is again a child of the cylinder.
        cmds.redo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 1)
        self.assertIn("pCube1", childrenNames(cylChildren))

        # Confirm that the cube's world transform has not changed.  Must
        # re-create the item, as its path has changed.
        cubeChildPath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/pCylinder1/pCube1")])
        cubeChildItem = ufe.Hierarchy.createItem(cubeChildPath)
        cubeChildT3d = ufe.Transform3d.transform3d(cubeChildItem)

        cubeWorld = cubeChildT3d.inclusiveMatrix()
        assertVectorAlmostEqual(
            self, cubeWorldListPre, matrixToList(cubeWorld), places=6)

        # Cube world y coordinate is currently 0.
        self.assertAlmostEqual(cubeWorld.matrix[3][1], 0)

        # Move the parent
        ufe.GlobalSelection.get().append(cylinderItem)

        cmds.move(0, 10, 0, relative=True)

        # Get the world space transform again.  The y component should
        # have incremented by 10.
        cubeWorld = cubeChildT3d.inclusiveMatrix()
        self.assertAlmostEqual(cubeWorld.matrix[3][1], 10)

        # Undo everything.
        for i in range(2):
            cmds.undo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 0)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2013', 'testParentToProxyShape only available in UFE preview version 0.2.13 and greater')
    def testParentToProxyShape(self):

        # Load a file with a USD hierarchy at least 2-levels deep.
        with OpenFileCtx("simpleHierarchy.ma"):

            # Create scene items for the proxy shape and the sphere.
            shapeSegment = mayaUtils.createUfePathSegment(
                "|world|mayaUsdProxy1|mayaUsdProxyShape1")
            shapePath = ufe.Path([shapeSegment])
            shapeItem = ufe.Hierarchy.createItem(shapePath)

            spherePath = ufe.Path(
                [shapeSegment, usdUtils.createUfePathSegment("/pCylinder1/pCube1/pSphere1")])
            sphereItem = ufe.Hierarchy.createItem(spherePath)

            # get the USD stage
            stage = mayaUsd.ufe.getStage(str(shapeSegment))

            # check GetLayerStack behavior
            self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

            # The sphere is not a child of the proxy shape.
            shapeHier = ufe.Hierarchy.hierarchy(shapeItem)
            shapeChildren = shapeHier.children()
            self.assertNotIn("pSphere1", childrenNames(shapeChildren))

            # Get its world space transform.
            sphereT3d = ufe.Transform3d.transform3d(sphereItem)
            sphereWorld = sphereT3d.inclusiveMatrix()
            sphereWorldListPre = matrixToList(sphereWorld)

            # Parent sphere to proxy shape in absolute mode (default), using UFE
            # path strings.Expect the exception happens
            cmds.parent("|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1/pCube1/pSphere1", "|mayaUsdProxy1|mayaUsdProxyShape1")

            # Confirm that the sphere is now a child of the proxy shape.
            shapeChildren = shapeHier.children()
            self.assertIn("pSphere1", childrenNames(shapeChildren))

            # Undo: the sphere is no longer a child of the proxy shape.
            cmds.undo()

            shapeChildren = shapeHier.children()
            self.assertNotIn("pSphere1", childrenNames(shapeChildren))

            # Redo: confirm that the sphere is again a child of the proxy shape.
            cmds.redo()

            shapeChildren = shapeHier.children()
            self.assertIn("pSphere1", childrenNames(shapeChildren))

            # Confirm that the sphere's world transform has not changed.  Must
            # re-create the item, as its path has changed.
            sphereChildPath = ufe.Path(
                [shapeSegment, usdUtils.createUfePathSegment("/pSphere1")])
            sphereChildItem = ufe.Hierarchy.createItem(sphereChildPath)
            sphereChildT3d = ufe.Transform3d.transform3d(sphereChildItem)

            sphereWorld = sphereChildT3d.inclusiveMatrix()
            assertVectorAlmostEqual(
                self, sphereWorldListPre, matrixToList(sphereWorld), places=6)

            # Undo.
            cmds.undo()

            shapeChildren = shapeHier.children()
            self.assertNotIn("pSphere1", childrenNames(shapeChildren))

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2013', 'testIllegalChild only available in UFE preview version 0.2.13 and greater')
    def testIllegalChild(self):
        '''Parenting an object to a descendant must report an error.'''

        with OpenFileCtx("simpleHierarchy.ma"):
            with self.assertRaises(RuntimeError):
                cmds.parent(
                    "|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1",
                    "|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1/pCube1")

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2016', 'testAlreadyChild only available in UFE preview version 0.2.16 and greater')
    def testAlreadyChild(self):
        '''Parenting an object to its current parent is a no-op.'''

        with OpenFileCtx("simpleHierarchy.ma"):
            shapeSegment = mayaUtils.createUfePathSegment(
                "|world|mayaUsdProxy1|mayaUsdProxyShape1")
            spherePath = ufe.Path(
                [shapeSegment,
                 usdUtils.createUfePathSegment("/pCylinder1/pCube1/pSphere1")])
            sphereItem = ufe.Hierarchy.createItem(spherePath)
            cylinderShapePath = ufe.Path(
                [shapeSegment,
                 usdUtils.createUfePathSegment("/pCylinder1/pCylinderShape1")])
            cylinderShapeItem = ufe.Hierarchy.createItem(cylinderShapePath)
            parentPath = ufe.Path(
                [shapeSegment, usdUtils.createUfePathSegment("/pCylinder1")])
            parentItem = ufe.Hierarchy.createItem(parentPath)

            parent = ufe.Hierarchy.hierarchy(parentItem)
            childrenPre = parent.children()

            # get the USD stage
            stage = mayaUsd.ufe.getStage(str(shapeSegment))

            # check GetLayerStack behavior
            self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

            # The sphere is not a child of the cylinder
            self.assertNotIn("pSphere1", childrenNames(childrenPre))

            # The cylinder shape is a child of the cylinder
            self.assertIn("pCylinderShape1", childrenNames(childrenPre))

            # Parent the sphere and the cylinder shape to the cylinder.  This
            # is a no-op for the cylinder shape, as it's already a child of the
            # cylinder, and the sphere is parented to the cylinder.
            cmds.parent(ufe.PathString.string(spherePath),
                        ufe.PathString.string(cylinderShapePath),
                        ufe.PathString.string(parentPath))

            children = parent.children()
            self.assertEqual(len(childrenPre)+1, len(children))
            self.assertIn("pSphere1", childrenNames(children))
            self.assertIn("pCylinderShape1", childrenNames(children))

            # Undo / redo
            cmds.undo()

            children = parent.children()
            self.assertEqual(len(childrenPre), len(children))
            self.assertNotIn("pSphere1", childrenNames(children))
            self.assertIn("pCylinderShape1", childrenNames(children))

            cmds.redo()

            children = parent.children()
            self.assertEqual(len(childrenPre)+1, len(children))
            self.assertIn("pSphere1", childrenNames(children))
            self.assertIn("pCylinderShape1", childrenNames(children))

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2018', 'testUnparentUSD only available in UFE preview version 0.2.18 and greater')
    def testUnparentUSD(self):
        '''Unparent USD node.'''

        with OpenFileCtx("simpleHierarchy.ma"):
            # Unparent a USD node
            cubePathStr =  '|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1/pCube1'
            cubePath = ufe.PathString.path(cubePathStr)
            cylinderItem = ufe.Hierarchy.createItem(ufe.PathString.path(
                '|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1'))
            proxyShapeItem = ufe.Hierarchy.createItem(
                ufe.PathString.path('|mayaUsdProxy1|mayaUsdProxyShape1'))
            proxyShape = ufe.Hierarchy.hierarchy(proxyShapeItem)
            cylinder = ufe.Hierarchy.hierarchy(cylinderItem)

            def checkUnparent(done):
                proxyShapeChildren = proxyShape.children()
                cylinderChildren = cylinder.children()
                self.assertEqual(
                    'pCube1' in childrenNames(proxyShapeChildren), done)
                self.assertEqual(
                    'pCube1' in childrenNames(cylinderChildren), not done)

            checkUnparent(done=False)

            cmds.parent(cubePathStr, world=True)
            checkUnparent(done=True)
            
            cmds.undo()
            checkUnparent(done=False)

            cmds.redo()
            checkUnparent(done=True)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2018', 'testUnparentMixed only available in UFE preview version 0.2.18 and greater')
    def testUnparentMultiStage(self):
        '''Unparent USD nodes in more than one stage.'''

        with OpenFileCtx("simpleHierarchy.ma"):
            # An early version of this test imported the same file into the
            # opened file.  Layers are then shared between the stages, because
            # they come from the same USD file, causing changes done below one
            # proxy shape to be seen in the other.  Import from another file.
            filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "test-samples", "parentCmd", "simpleSceneUSD_TRS.ma")
            cmds.file(filePath, i=True)

            # Unparent a USD node in each stage.  Unparenting Lambert node is
            # nonsensical, but demonstrates the functionality.
            cubePathStr1 = '|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1/pCube1'
            lambertPathStr2 = '|simpleSceneUSD_TRS_mayaUsdProxy1|simpleSceneUSD_TRS_mayaUsdProxyShape1,/initialShadingGroup/initialShadingGroup_lambert'

            cylinderItem1 = ufe.Hierarchy.createItem(ufe.PathString.path(
                '|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1'))
            shadingGroupItem2 = ufe.Hierarchy.createItem(
                ufe.PathString.path('|simpleSceneUSD_TRS_mayaUsdProxy1|simpleSceneUSD_TRS_mayaUsdProxyShape1,/initialShadingGroup'))
            proxyShapeItem1 = ufe.Hierarchy.createItem(ufe.PathString.path(
                '|mayaUsdProxy1|mayaUsdProxyShape1'))
            proxyShapeItem2 = ufe.Hierarchy.createItem(ufe.PathString.path(
                '|simpleSceneUSD_TRS_mayaUsdProxy1|simpleSceneUSD_TRS_mayaUsdProxyShape1'))
            cylinder1 = ufe.Hierarchy.hierarchy(cylinderItem1)
            shadingGroup2 = ufe.Hierarchy.hierarchy(shadingGroupItem2)
            proxyShape1 = ufe.Hierarchy.hierarchy(proxyShapeItem1)
            proxyShape2 = ufe.Hierarchy.hierarchy(proxyShapeItem2)

            def checkUnparent(done):
                proxyShape1Children   = proxyShape1.children()
                proxyShape2Children   = proxyShape2.children()
                cylinder1Children     = cylinder1.children()
                shadingGroup2Children = shadingGroup2.children()
                self.assertEqual(
                    'pCube1' in childrenNames(proxyShape1Children), done)
                self.assertEqual(
                    'pCube1' in childrenNames(cylinder1Children), not done)
                self.assertEqual(
                    'initialShadingGroup_lambert' in childrenNames(proxyShape2Children), done)
                self.assertEqual(
                    'initialShadingGroup_lambert' in childrenNames(shadingGroup2Children), not done)

            checkUnparent(done=False)

            # Use relative parenting, else trying to keep absolute world
            # position of Lambert node fails (of course).
            cmds.parent(cubePathStr1, lambertPathStr2, w=True, r=True)
            checkUnparent(done=True)
            
            cmds.undo()
            checkUnparent(done=False)

            cmds.redo()
            checkUnparent(done=True)
