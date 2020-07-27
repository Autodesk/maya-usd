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
from mayaUsd import lib as mayaUsdLib


class MayaUsdPythonCacheIdTestCase(unittest.TestCase):
    """
    Test cases for Cache Id
    """
    def __init__(self, *args, **kwargs):
        super(MayaUsdPythonCacheIdTestCase, self).__init__(*args, **kwargs)

        self.filePath = None
        self.SC = pxr.UsdUtils.StageCache.Get()

    def setUp(self):

        cmds.file(force=True, new=True)
        cmds.loadPlugin("mayaUsdPlugin", quiet=True)
        self.assertTrue(
            cmds.pluginInfo("mayaUsdPlugin", query=True, loaded=True))

        tmpfile = tempfile.NamedTemporaryFile(delete=True, suffix=".usda")
        tmpfile.close()
        self.filePath = tmpfile.name

        # Define a simple usd file for testing
        tempStage = Usd.Stage.CreateNew(self.filePath)
        UsdGeom.Xform.Define(tempStage, '/hello')
        UsdGeom.Sphere.Define(tempStage, '/hello/world')
        self.assertTrue(tempStage.GetRootLayer().Save())

        # Clear the cache before testing
        self.SC.Clear()

    def tearDown(self):
        """Unload plugin, new Maya scene, reset class member variables."""

        cmds.file(force=True, new=True)
        cmds.unloadPlugin("mayaUsdPlugin", force=True)

        os.remove(self.filePath)
        self.filePath = None

    def testCacheIdLoadFromFile(self):
        # Create the Proxy node and load the usd file
        shapeNode = cmds.createNode('mayaUsdProxyShape')
        cmds.setAttr('{}.filePath'.format(shapeNode),
                     self.filePath,
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
            cachedStage = Usd.Stage.Open(self.filePath)
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
                     self.filePath,
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
                     self.filePath,
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
            cachedStage = Usd.Stage.Open(self.filePath)
        stageId = self.SC.GetId(cachedStage).ToLongInt()

        self.assertGreaterEqual(stageId, 0)

        # Create the proxy node and load using the stage id
        shapeNode = cmds.createNode('mayaUsdProxyShape')
        cmds.setAttr('{}.stageCacheId'.format(shapeNode), stageId)

        # Save the file, reset and reopen it
        cmds.file(rename=self.filePath)
        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)
        cmds.file(self.filePath, open=True)

        # Check that the stageCacheId reset to -1 
        self.assertEqual(cmds.getAttr('{}.stageCacheId'.format(shapeNode)), -1)