#!/usr/bin/env python

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

import fixturesUtils
import testUtils
import mayaUsd_createStageWithNewLayer

import mayaUsd.lib

import mayaUtils
import mayaUsd.ufe

from pxr import Usd, Sdf

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import ufe

import unittest

import os

class CopyLayerPrimsTestCase(unittest.TestCase):
    '''
    Test the copyLayerPrims utility function, used in duplicate-to-USD.
    '''

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

    def _createStageAndLayer(self):
        '''Create and return an in-memory stage and its root layer.'''
        stage = Usd.Stage.CreateInMemory()
        self.assertIsNotNone(stage)
        layer = stage.GetRootLayer()
        self.assertIsNotNone(layer)
        return stage, layer
    
    def _createPrims(self, stage, srcPathAndTypes):
        '''Prims to be created. Might not necessarily all be copied.'''
        for srcPath, typeName in srcPathAndTypes.items():
            prim = stage.DefinePrim(Sdf.Path(srcPath), typeName)
            self.assertTrue(prim, 'Expected to create prim %s with type %s' % (srcPath, typeName))

    def _createTargets(self, stage, targets, asConnections):
        '''Creates the relationships described in the map (dict) of source to target.'''
        for srcPath, dstPath in targets.items():
            srcPrim = stage.GetPrimAtPath(Sdf.Path(srcPath))
            if asConnections:
                attr = srcPrim.CreateAttribute('tunnel', Sdf.ValueTypeNames.String, True)
                attr.AddConnection(Sdf.Path(dstPath))
            else:
                rel = srcPrim.CreateRelationship('arrow')
                rel.AddTarget(Sdf.Path(dstPath))

    def _createRelationships(self, stage, relations):
        '''Creates the relationships described in the map (dict) of source to target.'''
        self._createTargets(stage, relations, asConnections=False)

    def _createConnections(self, stage, connections):
        '''Creates the connections described in the map (dict) of source to target.'''
        self._createTargets(stage, connections, asConnections=True)

    def _createStageLayerAndPrims(self, srcPathAndTypes):
        '''Create and return an in-memory stage and its root layer and creates prims.'''
        stage, layer = self._createStageAndLayer()
        for srcPath, typeName in srcPathAndTypes.items():
            prim = stage.DefinePrim(Sdf.Path(srcPath), typeName)
            self.assertTrue(prim, 'Expected to create prim %s with type %s' % (srcPath, typeName))
        return stage, layer
    
    def _verifyDestinationPrims(self, stage, copiedPrims, srcPathAndTypes, expected):
        '''Verify that the expected prims were created.'''
        self.assertIsNotNone(copiedPrims)
        self.assertEqual(len(copiedPrims), len(expected))
        for srcPath, expectedDstPath in expected.items():
            self.assertIn(srcPath, srcPathAndTypes)
            self.assertIn(Sdf.Path(srcPath), copiedPrims)
            
            dstPath = copiedPrims[Sdf.Path(srcPath)]
            self.assertEqual(dstPath, Sdf.Path(expectedDstPath))

            dstPrim = stage.GetPrimAtPath(dstPath)
            self.assertTrue(dstPrim)
            self.assertEqual(dstPrim.GetTypeName(), srcPathAndTypes[srcPath])

    def _verifyDestinationTargets(self, stage, expected, asConnections):
        '''Verify that the prims have the expected targets.'''
        for dstPath, expectedTarget in expected.items():
            dstPrim = stage.GetPrimAtPath(Sdf.Path(dstPath))
            self.assertTrue(dstPrim)
            if asConnections:
                attr = dstPrim.GetAttribute('tunnel')
                self.assertIsNotNone(attr)
                self.assertTrue(attr)
                targets = attr.GetConnections()
            else:
                rel = dstPrim.GetRelationship('arrow')
                self.assertIsNotNone(rel)
                self.assertTrue(rel)
                targets = rel.GetTargets()
            self.assertIn(Sdf.Path(expectedTarget), targets)

    def _verifyDestinationRelationships(self, stage, expected):
        '''Verify that the relationships have the expected targets.'''
        return self._verifyDestinationTargets(stage, expected, asConnections=False)

    def _verifyDestinationConnections(self, stage, expected):
        '''Verify that the connections have the expected targets.'''
        return self._verifyDestinationTargets(stage, expected, asConnections=True)

    def testCopyLayerPrimsSimple(self):
        '''Copy a layer containing a cube.'''

        # Prims to be created. Might not necessarily all be copied
        srcPrims = {
            "/hello":   "Cube",
            "/ignored": "Capsule",
        }

        # Map of destination prim path indexed by the corresponding source path.
        expectedDstPrims = {
            "/hello" : "/hello",
        }

        # Create the source stage, layer and prims.
        srcStage, srcLayer = self._createStageLayerAndPrims(srcPrims)

        # Create the destination stage and layer.
        dstStage, dstLayer = self._createStageAndLayer()

        # Copy the desired prims for the test.
        toCopy = [Sdf.Path("/hello")]
        srcParentPath = Sdf.Path("/")
        dstParentPath = Sdf.Path("/")
        followRelationships = True
        copiedPrims = mayaUsd.lib.copyLayerPrims(srcStage, srcLayer, srcParentPath,
                                                dstStage, dstLayer, dstParentPath,
                                                toCopy, followRelationships)
        
        # Verify the results.
        self._verifyDestinationPrims(dstStage, copiedPrims, srcPrims, expectedDstPrims)

    def _runTestCopyLayerPrimsWithTarget(self, followTargets, asConnections):
        '''Copy a layer containing a cube. May or may not follow the target to the sphere.'''

        # Prims to be created. Might not necessarily all be copied
        srcPrims = {
            "/hello":   "Cube",
            "/ignored": "Capsule",
            "/related": "Sphere",
        }

        # Map of source to target.
        targets = {
            "/hello": "/related",
        }

        # Map of destination prim path indexed by the corresponding source path.
        # One map when targets are followed, one when not.
        expectedDstForFollow = {
            True: {
                "/hello" :  "/hello",
                "/related": "/related"
            },
            False: {
                "/hello" :  "/hello",
            }
        }

        # Map of relationship targets indexed by the destination path that
        # containing the targeting 'arrow' relationship or 'tunnel' connection.
        expectedTargetsForFollow = {
            True: {
                "/hello" :  "/related",
            },
            False: {},
        }

        toCopy = [Sdf.Path("/hello")]
        srcParentPath = Sdf.Path("/")
        dstParentPath = Sdf.Path("/")

        # Create the source stage, layer and prims.
        srcStage, srcLayer = self._createStageLayerAndPrims(srcPrims)
        self._createTargets(srcStage, targets, asConnections=asConnections)

        # Create the destination stage and layer.
        dstStage, dstLayer = self._createStageAndLayer()

        # Copy the desired prims for the test.
        copiedPrims = mayaUsd.lib.copyLayerPrims(srcStage, srcLayer, srcParentPath,
                                                dstStage, dstLayer, dstParentPath,
                                                toCopy, followTargets)
        
        # Verify the results.
        expectedDstPrims = expectedDstForFollow[followTargets]
        self._verifyDestinationPrims(dstStage, copiedPrims, srcPrims, expectedDstPrims)

        expectedDstTargets = expectedTargetsForFollow[followTargets]
        self._verifyDestinationTargets(dstStage, expectedDstTargets, asConnections=asConnections)

    def testCopyLayerPrimsWithFollowedRelation(self):
        '''Copy a layer containing a cube, following the relationship to the sphere.'''
        self._runTestCopyLayerPrimsWithTarget(followTargets=True, asConnections=False)

    def testCopyLayerPrimsWithIgnoredRelation(self):
        '''Copy a layer containing a cube, ignoring the relationship to the sphere.'''
        self._runTestCopyLayerPrimsWithTarget(followTargets=False, asConnections=False)

    def testCopyLayerPrimsWithFollowedConnection(self):
        '''Copy a layer containing a cube, following the conneection to the sphere.'''
        self._runTestCopyLayerPrimsWithTarget(followTargets=True, asConnections=True)

    def testCopyLayerPrimsWithIgnoredConnection(self):
        '''Copy a layer containing a cube, ignoring the conneection to the sphere.'''
        self._runTestCopyLayerPrimsWithTarget(followTargets=False, asConnections=True)

    def testCopyLayerPrimsNested(self):
        '''Copy a layer containing a cube and a nested cone.'''

        # Prims to be created. Might not necessarily all be copied
        srcPrims = {
            "/hello":       "Cube",
            "/hello/bye":   "Cone",
            "/ignored": "Capsule",
        }

        # Map of destination prim path indexed by the corresponding source path.
        expectedDstPrims = {
            "/hello" :      "/hello",
            "/hello/bye" :  "/hello/bye",
        }

        # Create the source stage, layer and prims.
        srcStage, srcLayer = self._createStageLayerAndPrims(srcPrims)

        # Create the destination stage and layer.
        dstStage, dstLayer = self._createStageAndLayer()

        # Copy the desired prims for the test.
        toCopy = Sdf.Path("/hello")
        srcParentPath = Sdf.Path("/")
        dstParentPath = Sdf.Path("/")
        followRelationships = True
        copiedPrims = mayaUsd.lib.copyLayerPrim(srcStage, srcLayer, srcParentPath,
                                                dstStage, dstLayer, dstParentPath,
                                                toCopy, followRelationships)
        
        # Verify the results.
        self._verifyDestinationPrims(dstStage, copiedPrims, srcPrims, expectedDstPrims)

    def testCopyLayerPrimsNestedLower(self):
        '''Copy a layer containing a cube and a nested cone under a non-root destination prim.'''

        # Prims to be created. Might not necessarily all be copied
        srcPrims = {
            "/hello":       "Cube",
            "/hello/bye":   "Cone",
            "/ignored": "Capsule",
        }

        # Map of destination prim path indexed by the corresponding source path.
        expectedDstPrims = {
            "/hello" :      "/there/hello",
            "/hello/bye" :  "/there/hello/bye",
        }

        # Create the source stage, layer and prims.
        srcStage, srcLayer = self._createStageLayerAndPrims(srcPrims)

        # Create the destination stage, layer and destination prim.
        dstStage, dstLayer = self._createStageAndLayer()
        dstStage.DefinePrim("/there")

        # Copy the desired prims for the test.
        toCopy = Sdf.Path("/hello")
        srcParentPath = Sdf.Path("/")
        dstParentPath = Sdf.Path("/there")
        followRelationships = True
        copiedPrims = mayaUsd.lib.copyLayerPrim(srcStage, srcLayer, srcParentPath,
                                                dstStage, dstLayer, dstParentPath,
                                                toCopy, followRelationships)
        
        # Verify the results.
        self._verifyDestinationPrims(dstStage, copiedPrims, srcPrims, expectedDstPrims)

    def _runTestCopyLayerPrimsNestedToLowerWithTarget(self, followTargets, asConnections):
        '''
        Copy a layer containing a cube and a nested cone under a non-root destination prim.
        There is also a target that may or may not be followed depending on options.
        '''

        # Prims to be created. Might not necessarily all be copied
        srcPrims = {
            "/hello":       "Cube",
            "/hello/bye":   "Cone",
            "/ignored":     "Capsule",
            "/related":     "Sphere",
        }

        # Map of source to target.
        targets = {
            "/hello": "/related",
        }

        # Map of destination prim path indexed by the corresponding source path.
        # One map when targets are followed, one when not.
        expectedDstForFollow = {
            True: {
                "/hello" :      "/there/hello",
                "/hello/bye" :  "/there/hello/bye",
                "/related":     "/there/related"
            },
            False: {
                "/hello" :      "/there/hello",
                "/hello/bye" :  "/there/hello/bye",
            }
        }

        # Map of relationship targets indexed by the destination path that
        # containing the targeting 'arrow' relationship or 'tunnel' connection.
        expectedTargetsForFollow = {
            True: {
                "/there/hello" :    "/related",
            },
            False: {},
        }

        # Create the source stage, layer and prims.
        srcStage, srcLayer = self._createStageLayerAndPrims(srcPrims)
        self._createTargets(srcStage, targets, asConnections=asConnections)

        toCopy = [Sdf.Path("/hello")]
        srcParentPath = Sdf.Path("/")
        dstParentPath = Sdf.Path("/there")

        # Create the destination stage, layer and destination prim.
        dstStage, dstLayer = self._createStageAndLayer()
        dstStage.DefinePrim("/there")

        # Copy the desired prims for the test.
        copiedPrims = mayaUsd.lib.copyLayerPrims(srcStage, srcLayer, srcParentPath,
                                                dstStage, dstLayer, dstParentPath,
                                                toCopy, followTargets)
        
        # Verify the results.
        expectedDstPrims = expectedDstForFollow[followTargets]
        self._verifyDestinationPrims(dstStage, copiedPrims, srcPrims, expectedDstPrims)
        
        expectedDstTargets = expectedTargetsForFollow[followTargets]
        self._verifyDestinationTargets(dstStage, expectedDstTargets, asConnections=asConnections)

    def testCopyLayerPrimsNestedToLowerWithRelation(self):
        '''
        Copy a layer containing a cube and a nested cone under a non-root destination prim.
        There is also a relationship that is followed.
        '''
        self._runTestCopyLayerPrimsNestedToLowerWithTarget(followTargets=True, asConnections=False)
    
    def testCopyLayerPrimsNestedToLowerIgnoreRelation(self):
        '''
        Copy a layer containing a cube and a nested cone under a non-root destination prim.
        There is also a relationship that ignored.
        '''
        self._runTestCopyLayerPrimsNestedToLowerWithTarget(followTargets=False, asConnections=False)
    
    def testCopyLayerPrimsNestedToLowerWithConnection(self):
        '''
        Copy a layer containing a cube and a nested cone under a non-root destination prim.
        There is also a connection that is followed.
        '''
        self._runTestCopyLayerPrimsNestedToLowerWithTarget(followTargets=True, asConnections=True)
    
    def testCopyLayerPrimsNestedToLowerIgnoreConnection(self):
        '''
        Copy a layer containing a cube and a nested cone under a non-root destination prim.
        There is also a connection that ignored.
        '''
        self._runTestCopyLayerPrimsNestedToLowerWithTarget(followTargets=False, asConnections=True)
    
    def _runTestCopyLayerPrimsChainedRenaming(self, asConnections):
        '''
        Copy a layer containing three nested prims, each with a relation to a prim with
        a name ending with an increasing digit, which will collide in the destination.

        This will create a chain of renaming:
            r1 will be renamed to r2
            r2 will be renamed to r3
            r3 will be renamed to r4

        When handling the renamed targets, the test verifies that the target only follow one
        level of renaming. Meaning the correct result we expect is that r1 is now r2, r2 -> r3
        and r4 -> r4. If the code were wrong and applied all renaming mappings to all targets,
        all targets would incorrectly end-up pointing to r4.
        '''

        # Prims to be created. Might not necessarily all be copied
        srcPrims = {
            "/a":           "Cube",
            "/a/b":         "Cube",
            "/a/b/c":       "Cube",
            "/ignored":     "Capsule",
            "/r1":          "Cone",
            "/r2":          "Capsule",
            "/r3":          "Sphere",
        }

        # Map of source to targets.
        #
        # Note: due to the order of traversal, the targets of the lowest
        #       prims are copied first, so to create the chain of renaming, we
        #       need to have r1 be in the lowest prim, not the highest.
        targets = {
            "/a":       "/r3",
            "/a/b":     "/r2",
            "/a/b/c":   "/r1",
        }

        # Map of destination prim path indexed by the corresponding source path.
        expectedDstPrims = {
            "/a" :      "/a",
            "/a/b" :    "/a/b",
            "/a/b/c":   "/a/b/c",
            "/r1":      "/r2",
            "/r2":      "/r3",
            "/r3":      "/r4",
        }

        # Map of relationship targets indexed by the destination path that
        # containing the targeting 'arrow' relationship or 'tunnel' connection.
        expectedDstTargets = {
            "/a" :      "/r4",
            "/a/b" :    "/r3",
            "/a/b/c":   "/r2",
        }

        # Create the source stage, layer and prims.
        srcStage, srcLayer = self._createStageLayerAndPrims(srcPrims)
        self._createTargets(srcStage, targets, asConnections=asConnections)

        toCopy = [Sdf.Path("/a")]
        srcParentPath = Sdf.Path("/")
        dstParentPath = Sdf.Path("/")
        followRelationships = True

        # Create the destination stage, layer and a prim that will collide
        # with a relationship target, which will trigger the chain of renaming.
        dstStage, dstLayer = self._createStageAndLayer()
        dstStage.DefinePrim("/r1")

        # Copy the desired prims for the test.
        copiedPrims = mayaUsd.lib.copyLayerPrims(srcStage, srcLayer, srcParentPath,
                                                 dstStage, dstLayer, dstParentPath,
                                                 toCopy, followRelationships)
        
        # Verify the results.
        self._verifyDestinationPrims(dstStage, copiedPrims, srcPrims, expectedDstPrims)
        self._verifyDestinationTargets(dstStage, expectedDstTargets, asConnections=asConnections)

    def testCopyLayerPrimsChainedRelationRenaming(self):
        '''
        Copy a layer containing three nested prims, each with a relation to a prim with
        a name ending with an increasing digit, which will collide in the destination.
        '''
        self._runTestCopyLayerPrimsChainedRenaming(asConnections=False)

    def testCopyLayerPrimsChainedConnectionRenaming(self):
        '''
        Copy a layer containing three nested prims, each with a connection to a prim with
        a name ending with an increasing digit, which will collide in the destination.
        '''
        self._runTestCopyLayerPrimsChainedRenaming(asConnections=True)

    def _runTestCopyLayerPrimsChainedTargets(self, asConnections):
        '''
        Copy a layer containing a prim with a target to a prim with another target.
        Verify that all related prims in the chain get copied.

        Also, the targets form a circle, verify this does not cause infinite
        recursion.        
        '''

        # Prims to be created. Might not necessarily all be copied
        srcPrims = {
            "/hello":       "Cube",
            "/ignored":     "Capsule",
            "/r1":          "Cone",
            "/r2":          "Sphere",
        }

        # Map of source to targets.
        targets = {
            "/hello":   "/r1",
            "/r1":      "/r2",
            "/r2":      "/r1",
        }

        # Map of destination prim path indexed by the corresponding source path.
        expectedDstPrims = {
            "/hello" :  "/hello",
            "/r1":      "/r1",
            "/r2":      "/r2",
        }

        # Map of relationship targets indexed by the destination path that
        # containing the targeting 'arrow' relationship or 'tunnel' connection.
        expectedDstTargets = {
            "/hello" :  "/r1",
            "/r1":      "/r2",
            "/r2":      "/r1",
        }

        # Create the source stage, layer and prims.
        srcStage, srcLayer = self._createStageLayerAndPrims(srcPrims)
        self._createTargets(srcStage, targets, asConnections=asConnections)

        toCopy = [Sdf.Path("/hello")]
        srcParentPath = Sdf.Path("/")
        dstParentPath = Sdf.Path("/")
        followRelationships = True

        # Create the destination stage, layer and a prim that will collide
        # with a relationship target, which will trigger the chain of renaming.
        dstStage, dstLayer = self._createStageAndLayer()

        # Copy the desired prims for the test.
        copiedPrims = mayaUsd.lib.copyLayerPrims(srcStage, srcLayer, srcParentPath,
                                                 dstStage, dstLayer, dstParentPath,
                                                 toCopy, followRelationships)
        
        # Verify the results.
        self._verifyDestinationPrims(dstStage, copiedPrims, srcPrims, expectedDstPrims)
        self._verifyDestinationTargets(dstStage, expectedDstTargets, asConnections=asConnections)

    def testCopyLayerPrimsChainedRelations(self):
        '''
        Copy a layer containing with a relation to a prim with another relation.
        Verify that all related prims in the chain get copied.

        Also, the relations form a circle, verify this does not cause infinite
        recursion.        
        '''
        self._runTestCopyLayerPrimsChainedTargets(asConnections=False)

    def testCopyLayerPrimsChainedConnections(self):
        '''
        Copy a layer containing with a connection to a prim with another connection.
        Verify that all related prims in the chain get copied.

        Also, the connections form a circle, verify this does not cause infinite
        recursion.        
        '''
        self._runTestCopyLayerPrimsChainedTargets(asConnections=True)

    def testCopyLayerPrimsWithMixedRelationAndConnection(self):
        '''
        Copy a layer containing a prim with both relationship and connection.

        Also, the targets form a circle, verify this does not cause infinite
        recursion.        
        '''

        # Prims to be created. Might not necessarily all be copied
        srcPrims = {
            "/hello":       "Cube",
            "/ignored":     "Capsule",
            "/r1":          "Cone",
            "/r2":          "Sphere",
        }

        # Map of source to targets.
        relationships = {
            "/hello":   "/r1",
            "/r1":      "/r2",
        }

        # Map of source to targets.
        connections = {
            "/r2":      "/r1",
        }

        # Map of destination prim path indexed by the corresponding source path.
        expectedDstPrims = {
            "/hello" :  "/hello",
            "/r1":      "/r1",
            "/r2":      "/r2",
        }

        # Map of relationship targets indexed by the destination path that
        # containing the targeting 'arrow' relationship.
        expectedDstRelations = {
            "/hello" :  "/r1",
            "/r1":      "/r2",
        }

        # Map of relationship targets indexed by the destination path that
        # containing the targeting 'tunnel' connection.
        expectedDstConnections = {
            "/r2":      "/r1",
        }

        # Create the source stage, layer and prims.
        srcStage, srcLayer = self._createStageLayerAndPrims(srcPrims)
        self._createRelationships(srcStage, relationships)
        self._createConnections(srcStage, connections)

        toCopy = [Sdf.Path("/hello")]
        srcParentPath = Sdf.Path("/")
        dstParentPath = Sdf.Path("/")
        followRelationships = True

        # Create the destination stage, layer and a prim that will collide
        # with a relationship target, which will trigger the chain of renaming.
        dstStage, dstLayer = self._createStageAndLayer()

        # Copy the desired prims for the test.
        copiedPrims = mayaUsd.lib.copyLayerPrims(srcStage, srcLayer, srcParentPath,
                                                 dstStage, dstLayer, dstParentPath,
                                                 toCopy, followRelationships)
        
        # Verify the results.
        self._verifyDestinationPrims(dstStage, copiedPrims, srcPrims, expectedDstPrims)
        self._verifyDestinationRelationships(dstStage, expectedDstRelations)
        self._verifyDestinationConnections(dstStage, expectedDstConnections)


if __name__ == '__main__':
    unittest.main(verbosity=2)
