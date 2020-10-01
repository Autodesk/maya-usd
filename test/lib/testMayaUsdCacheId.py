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

import unittest
import os
import tempfile

import pxr
from pxr import Usd, UsdGeom, UsdUtils

from maya import cmds
from maya import standalone
from mayaUsd import lib as mayaUsdLib

import fixturesUtils

class MayaUsdPythonCacheIdTestCase(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)
        cls.SC = pxr.UsdUtils.StageCache.Get()
        cls.mayafilePath = os.path.join(inputPath, "MayaUsdPythonCacheIdTestCase", "MayaUsdPythonCacheIdTestCase.ma")
        cls.usdfilePath =  os.path.join(inputPath, "MayaUsdPythonCacheIdTestCase", "helloworld.usda")
        cmds.file(new=True, force=True)

        # Define a simple usd file for testing
        tempStage = Usd.Stage.CreateNew(cls.usdfilePath)
        UsdGeom.Xform.Define(tempStage, '/hello')
        UsdGeom.Sphere.Define(tempStage, '/hello/world')
        tempStage.GetRootLayer().Save()

        # Clear the cache before testing
        cls.SC.Clear()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()
        os.remove(cls.mayafilePath)
        os.remove(cls.usdfilePath)
        cls.mayafilePath = None
        cls.usdfilePath = None

    def testCacheIdLoadFromFile(self):
        # Create the Proxy node and load the usd file
        shapeNode = cmds.createNode('mayaUsdProxyShape')
        cmds.setAttr('{}.filePath'.format(shapeNode),
                     self.usdfilePath,
                     type='string')
        shapeStage = mayaUsdLib.GetPrim(shapeNode).GetStage()

        # Get the cache ID and load the stage from the cache
        cacheIdValue = cmds.getAttr('{}.outStageCacheId'.format(shapeNode))
        cacheId = pxr.Usd.StageCache.Id.FromLongInt(cacheIdValue)
        self.assertTrue(self.SC.Contains(cacheId))
        cacheStage = self.SC.Find(cacheId)

        # Check they are the same
        self.assertEqual(shapeStage, cacheStage)

    def testCacheIdLoadFromCacheId(self):
        # Open the stage and get the cache Id
        with pxr.Usd.StageCacheContext(self.SC):
            cachedStage = Usd.Stage.Open(self.usdfilePath)
        stageId = self.SC.GetId(cachedStage).ToLongInt()

        # Create the proxy node and load using the stage id
        shapeNode = cmds.createNode('mayaUsdProxyShape')
        cmds.setAttr('{}.stageCacheId'.format(shapeNode), stageId)
        shapeStage = mayaUsdLib.GetPrim(shapeNode).GetStage()

        # Get the cache ID and load the stage from the cache
        cacheIdValue = cmds.getAttr('{}.outStageCacheId'.format(shapeNode))
        cacheId = pxr.Usd.StageCache.Id.FromLongInt(cacheIdValue)
        self.assertTrue(self.SC.Contains(cacheId))
        cacheStage = self.SC.Find(cacheId)

        # Check they are the same
        self.assertEqual(shapeStage, cacheStage)

    def testCacheIdLoadFromInMemory(self):
        # Create the Proxy node and get the in-memory stage
        shapeNode = cmds.createNode('mayaUsdProxyShape')
        shapeStage = mayaUsdLib.GetPrim(shapeNode).GetStage()

        # Get the cache ID and load the stage from the cache
        cacheIdValue = cmds.getAttr('{}.outStageCacheId'.format(shapeNode))
        cacheId = pxr.Usd.StageCache.Id.FromLongInt(cacheIdValue)
        self.assertTrue(self.SC.Contains(cacheId))
        cacheStage = self.SC.Find(cacheId)

        # Check they are the same
        self.assertEqual(shapeStage, cacheStage)

    def testCacheIdConnectingTwoProxyNodes(self):
        # Create the 2 Proxy nodes and load a stage from file
        shapeNodeA = cmds.createNode('mayaUsdProxyShape')
        cmds.setAttr('{}.filePath'.format(shapeNodeA),
                     self.usdfilePath,
                     type='string')
        shapeStageA = mayaUsdLib.GetPrim(shapeNodeA).GetStage()
        shapeNodeB = cmds.createNode('mayaUsdProxyShape')

        #Connect them
        cmds.connectAttr('{}.outStageCacheId'.format(shapeNodeA),
                         '{}.stageCacheId'.format(shapeNodeB))

        shapeStageB = mayaUsdLib.GetPrim(shapeNodeA).GetStage()

        # Check they are the same
        self.assertEqual(shapeStageA, shapeStageB)

        # Get the cache ID and load the stage from the cache
        cacheIdValue = cmds.getAttr('{}.outStageCacheId'.format(shapeNodeB))
        cacheId = pxr.Usd.StageCache.Id.FromLongInt(cacheIdValue)
        self.assertTrue(self.SC.Contains(cacheId))
        cacheStageB = self.SC.Find(cacheId)

        # Check they are the same
        self.assertEqual(shapeStageA, cacheStageB)

    def testCacheIdInStageData(self):
        # Create the 2 Proxy nodes and load a stage from file
        shapeNodeA = cmds.createNode('mayaUsdProxyShape')
        cmds.setAttr('{}.filePath'.format(shapeNodeA),
                     self.usdfilePath,
                     type='string')
        shapeStageA = mayaUsdLib.GetPrim(shapeNodeA).GetStage()
        shapeNodeB = cmds.createNode('mayaUsdProxyShape')

        #Connect them
        cmds.connectAttr('{}.outStageData'.format(shapeNodeA),
                         '{}.inStageData'.format(shapeNodeB))

        shapeStageB = mayaUsdLib.GetPrim(shapeNodeA).GetStage()

        # Check they are the same
        self.assertEqual(shapeStageA, shapeStageB)

        # Get the cache ID and load the stage from the cache
        cacheIdValue = cmds.getAttr('{}.outStageCacheId'.format(shapeNodeB))
        cacheId = pxr.Usd.StageCache.Id.FromLongInt(cacheIdValue)
        self.assertTrue(self.SC.Contains(cacheId))
        cacheStageB = self.SC.Find(cacheId)

        # Check they are the same
        self.assertEqual(shapeStageA, cacheStageB)

    def testCacheIdNotStoredInFile(self):
        # Open the stage and get the cache Id
        with pxr.Usd.StageCacheContext(self.SC):
            cachedStage = Usd.Stage.Open(self.usdfilePath)
        stageId = self.SC.GetId(cachedStage).ToLongInt()

        self.assertGreaterEqual(stageId, 0)

        # Create the proxy node and load using the stage id
        shapeNode = cmds.createNode('mayaUsdProxyShape')
        cmds.setAttr('{}.stageCacheId'.format(shapeNode), stageId)

        # Save the file, reset and reopen it
        cmds.file(rename=self.mayafilePath)
        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)
        cmds.file(self.mayafilePath, open=True)

        # Check that the stageCacheId reset to -1
        self.assertEqual(cmds.getAttr('{}.stageCacheId'.format(shapeNode)), -1)

    def testCacheIdDisconnect(self):
        # Create the 2 Proxy nodes and load a stage from file
        shapeNodeA = cmds.createNode('mayaUsdProxyShape')
        cmds.setAttr('{}.filePath'.format(shapeNodeA),
                     self.usdfilePath,
                     type='string')
        shapeStageA = mayaUsdLib.GetPrim(shapeNodeA).GetStage()
        shapeNodeB = cmds.createNode('mayaUsdProxyShape')

        #Connect them
        cmds.connectAttr('{}.outStageCacheId'.format(shapeNodeA),
                         '{}.stageCacheId'.format(shapeNodeB))

        shapeStageB = mayaUsdLib.GetPrim(shapeNodeA).GetStage()

        # Check they are the same
        self.assertEqual(shapeStageA, shapeStageB)

        # Diconnect them
        cmds.disconnectAttr('{}.outStageCacheId'.format(shapeNodeA),
                            '{}.stageCacheId'.format(shapeNodeB))

        # Check that the stageCacheId reset to -1
        self.assertEqual(cmds.getAttr('{}.stageCacheId'.format(shapeNodeB)),
                         -1)
