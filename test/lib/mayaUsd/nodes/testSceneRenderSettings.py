#!/usr/bin/env mayapy
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

from maya import cmds
from maya import standalone

from mayaUsd.lib import SceneRenderSettings

from pxr import UsdGeom, UsdRender, UsdUtils

import fixturesUtils

import os
import tempfile
import unittest


class testSceneRenderSettings(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    # ------------------------------------------------------------------
    # Node existence and singleton
    # ------------------------------------------------------------------

    def testNodeExistsOnStartup(self):
        '''The singleton node should exist after plugin load.'''
        path = SceneRenderSettings.find()
        self.assertTrue(len(path) > 0, "SceneRenderSettings node not found")
        self.assertTrue(cmds.objExists(path))

    def testSingleton(self):
        '''find called twice should return the same node; only one instance exists.'''
        path1 = SceneRenderSettings.find()
        path2 = SceneRenderSettings.find()
        self.assertEqual(path1, path2)

        nodes = cmds.ls(type='mayaUsdSceneRenderSettings')
        self.assertEqual(len(nodes), 1,
                         "Expected exactly one SceneRenderSettings node, "
                         "found %d" % len(nodes))

    # ------------------------------------------------------------------
    # Default stage structure
    # ------------------------------------------------------------------

    def testDefaultStageStructure(self):
        '''The default stage should have /Render scope and /Render/SceneRenderSettings prim.'''
        stage = SceneRenderSettings.getUsdStage()
        self.assertIsNotNone(stage)

        renderPrim = stage.GetPrimAtPath('/Render')
        self.assertTrue(renderPrim.IsValid(), "/Render prim not found")
        self.assertTrue(renderPrim.IsA(UsdGeom.Scope))

        settingsPrim = stage.GetPrimAtPath('/Render/SceneRenderSettings')
        self.assertTrue(settingsPrim.IsValid(),
                        "/Render/SceneRenderSettings prim not found")
        self.assertTrue(settingsPrim.IsA(UsdRender.Settings))

    def testRenderSettingsPrimPathMetadata(self):
        '''Stage metadata should point to the default render settings prim.'''
        stage = SceneRenderSettings.getUsdStage()
        metadata = stage.GetMetadataByDictKey('', 'renderSettingsPrimPath')
        self.assertEqual(metadata, '/Render/SceneRenderSettings')

    # ------------------------------------------------------------------
    # Node properties
    # ------------------------------------------------------------------

    def testNodeIsLocked(self):
        '''The singleton shape and its parent transform should be locked.'''
        shapePath = SceneRenderSettings.find()
        self.assertTrue(cmds.lockNode(shapePath, query=True, lock=True)[0],
                        "Shape node should be locked")

        parentPath = cmds.listRelatives(shapePath, parent=True,
                                        fullPath=True)[0]
        self.assertTrue(cmds.lockNode(parentPath, query=True, lock=True)[0],
                        "Transform node should be locked")

    def testNodeHiddenInOutliner(self):
        '''The parent transform should be hidden in the outliner.'''
        shapePath = SceneRenderSettings.find()
        parentPath = cmds.listRelatives(shapePath, parent=True,
                                        fullPath=True)[0]
        self.assertTrue(cmds.getAttr(parentPath + '.hiddenInOutliner'),
                        "Transform should be hidden in outliner")

    # ------------------------------------------------------------------
    # Output attributes
    # ------------------------------------------------------------------

    def testOutStageCacheId(self):
        '''outStageCacheId should return a valid stage cache ID.'''
        shapePath = SceneRenderSettings.find()
        cacheId = cmds.getAttr(shapePath + '.outStageCacheId')
        self.assertGreaterEqual(cacheId, 0,
                                "Expected valid cache ID, got %d" % cacheId)

        # Verify we can retrieve the same stage from the cache.
        cachedStage = UsdUtils.StageCache.Get().Find(
            UsdUtils.StageCache.Id.FromLongInt(cacheId))
        apiStage = SceneRenderSettings.getUsdStage()
        self.assertEqual(cachedStage.GetRootLayer().identifier,
                         apiStage.GetRootLayer().identifier)

    # ------------------------------------------------------------------
    # Stage access
    # ------------------------------------------------------------------

    def testGetUsdStageConsistency(self):
        '''getUsdStage should return the same stage on repeated calls.'''
        stage1 = SceneRenderSettings.getUsdStage()
        stage2 = SceneRenderSettings.getUsdStage()
        self.assertIsNotNone(stage1)
        self.assertEqual(stage1.GetRootLayer().identifier,
                         stage2.GetRootLayer().identifier)

    def testGetDefaultRenderSettingsPrim(self):
        '''getDefaultRenderSettingsPrim should return the /Render/SceneRenderSettings prim.'''
        prim = SceneRenderSettings.getDefaultRenderSettingsPrim()
        self.assertTrue(prim.IsValid())
        self.assertEqual(prim.GetPath().pathString,
                         '/Render/SceneRenderSettings')
        self.assertTrue(prim.IsA(UsdRender.Settings))

    # ------------------------------------------------------------------
    # Node recreation on file new
    # ------------------------------------------------------------------

    def testNodeRecreatedAfterFileNew(self):
        '''The singleton should be recreated after file new.'''
        pathBefore = SceneRenderSettings.find()
        self.assertTrue(len(pathBefore) > 0)

        cmds.file(new=True, force=True)

        pathAfter = SceneRenderSettings.find()
        self.assertTrue(len(pathAfter) > 0,
                        "Node should be recreated after file new")

        # The new stage should have the default structure.
        stage = SceneRenderSettings.getUsdStage()
        self.assertTrue(
            stage.GetPrimAtPath('/Render/SceneRenderSettings').IsValid())

    # ------------------------------------------------------------------
    # Serialization round-trip
    # ------------------------------------------------------------------

    def testSerializationRoundTrip(self):
        '''Stage content should survive a save/open cycle.'''
        stage = SceneRenderSettings.getUsdStage()

        # Author a prim directly on the stage.
        UsdGeom.Xform.Define(stage, '/Render/TestContent')

        # Save.
        tmpFile = os.path.join(tempfile.mkdtemp(), 'testScene.ma')
        cmds.file(rename=tmpFile)
        cmds.file(save=True, type='mayaAscii')

        # Re-open.
        cmds.file(tmpFile, open=True, force=True)

        # Verify.
        stage2 = SceneRenderSettings.getUsdStage()
        self.assertIsNotNone(stage2)

        self.assertTrue(
            stage2.GetPrimAtPath('/Render/SceneRenderSettings').IsValid(),
            "Default render settings prim should survive serialization")
        self.assertTrue(
            stage2.GetPrimAtPath('/Render/TestContent').IsValid(),
            "Authored content should survive serialization")


if __name__ == '__main__':
    unittest.main(verbosity=2)
