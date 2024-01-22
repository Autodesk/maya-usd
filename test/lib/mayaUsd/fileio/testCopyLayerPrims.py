#!/usr/bin/env python

#
# Copyright 2021 Autodesk
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

    def _createRelationships(self, stage, relations):
        '''Creates the relationships described in the map (dict) of source to target.'''
        for srcPath, dstPath in relations.items():
            srcPrim = stage.GetPrimAtPath(Sdf.Path(srcPath))
            rel = srcPrim.CreateRelationship('arrow')
            rel.AddTarget(Sdf.Path(dstPath))

    def _createStageLayerAndPrims(self, srcPathAndTypes):
        '''Create and return an in-memory stage and its root layer and creates prims.'''
        stage, layer = self._createStageAndLayer()
        for srcPath, typeName in srcPathAndTypes.items():
            prim = stage.DefinePrim(Sdf.Path(srcPath), typeName)
            self.assertTrue(prim, 'Expected to create prim %s with type %s' % (srcPath, typeName))
        return stage, layer
    
    def _verifyDestinationPrims(self, stage, copiedPrims, srcPathAndTypes, expected):
        '''Veify that the expected prims were created.'''
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

    def testCopyLayerPrimsWithRelation(self):
        '''Copy a layer containing a cube. May or may not follow the relation to the sphere.'''

        # Prims to be created. Might not necessarily all be copied
        srcPrims = {
            "/hello":   "Cube",
            "/ignored": "Capsule",
            "/related": "Sphere",
        }

        # Map of source to target relationships.
        relationships = {
            "/hello": "/related",
        }

        # Map of destination prim path indexed by the corresponding source path.
        # One map when relationships are followed, one when not.
        expectedForFollow = {
            True: {
                "/hello" :  "/hello",
                "/related": "/related"
            },
            False: {
                "/hello" :  "/hello",
            }
        }

        # Create the source stage, layer and prims.
        srcStage, srcLayer = self._createStageLayerAndPrims(srcPrims)
        self._createRelationships(srcStage, relationships)

        toCopy = [Sdf.Path("/hello")]
        srcParentPath = Sdf.Path("/")
        dstParentPath = Sdf.Path("/")

        for followRelationships in [True, False]:
            # Create the destination stage and layer.
            dstStage, dstLayer = self._createStageAndLayer()

            # Copy the desired prims for the test.
            copiedPrims = mayaUsd.lib.copyLayerPrims(srcStage, srcLayer, srcParentPath,
                                                    dstStage, dstLayer, dstParentPath,
                                                    toCopy, followRelationships)
            
            # Verify the results.
            expectedDstPrims = expectedForFollow[followRelationships]
            self._verifyDestinationPrims(dstStage, copiedPrims, srcPrims, expectedDstPrims)

    def testCopyLayerPrimsNested(self):
        '''Copy a layer containing a cube and a nexted cone.'''

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
        '''Copy a layer containing a cube and a nexted cone under a non-root destination prim.'''

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

    def testCopyLayerPrimsNestedToLowerWithRelation(self):
        '''
        Copy a layer containing a cube and a nexted cone under a non-root destination prim.
        There is also a relationship that may or may not be followed depending on options.
        '''

        # Prims to be created. Might not necessarily all be copied
        srcPrims = {
            "/hello":       "Cube",
            "/hello/bye":   "Cone",
            "/ignored":     "Capsule",
            "/related":     "Sphere",
        }

        # Map of source to target relationships.
        relationships = {
            "/hello": "/related",
        }

        # Map of destination prim path indexed by the corresponding source path.
        # One map when relationships are followed, one when not.
        expectedForFollow = {
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

        # Create the source stage, layer and prims.
        srcStage, srcLayer = self._createStageLayerAndPrims(srcPrims)
        self._createRelationships(srcStage, relationships)

        toCopy = [Sdf.Path("/hello")]
        srcParentPath = Sdf.Path("/")
        dstParentPath = Sdf.Path("/there")

        for followRelationships in [True, False]:
            # Create the destination stage, layer and destination prim.
            dstStage, dstLayer = self._createStageAndLayer()
            dstStage.DefinePrim("/there")

            # Copy the desired prims for the test.
            copiedPrims = mayaUsd.lib.copyLayerPrims(srcStage, srcLayer, srcParentPath,
                                                    dstStage, dstLayer, dstParentPath,
                                                    toCopy, followRelationships)
            
            # Verify the results.
            expectedDstPrims = expectedForFollow[followRelationships]
            self._verifyDestinationPrims(dstStage, copiedPrims, srcPrims, expectedDstPrims)


if __name__ == '__main__':
    unittest.main(verbosity=2)
