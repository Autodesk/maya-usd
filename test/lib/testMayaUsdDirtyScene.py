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

import unittest
import mayaUtils

import ufe

import maya.cmds as cmds
import mayaUsd_createStageWithNewLayer
import mayaUsd

PROXY_NODE_TYPE = "mayaUsdProxyShapeBase"

def getMayaStage():
    proxyShapes = cmds.ls(type=PROXY_NODE_TYPE, long=True)
    shapePath = proxyShapes[0]
    stage = mayaUsd.lib.GetPrim(shapePath).GetStage()
    return shapePath, stage

def getCleanMayaStage():
    """ gets a stage that only has an anon layer """
    cmds.file(new=True, force=True)
    shapePath = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
    stage = mayaUsd.lib.GetPrim(shapePath).GetStage()
    return shapePath, stage

class DirtySceneCheck:
    def __init__(self, test):
        self.test = test

    def __enter__(self):
        # Set the scene NOT dirty.
        cmds.file(modified=False)

    def __exit__(self, type, value, traceback):
        # Verify that that the scene IS dirty after action.
        self.test.assertTrue(cmds.file(query=True, modified=True))

class MayaUsdDirtySceneTestCase(unittest.TestCase):
    """ Test that various actions will dirty the Maya scene."""

    @classmethod
    def setUpClass(cls):
        cmds.loadPlugin('mayaUsdPlugin')

    def testAnonymousLayers(self):
        shapePath, stage = getCleanMayaStage()
        rootLayer = stage.GetRootLayer()

        # Add anonymous layer -> dirties scene
        with DirtySceneCheck(self):
            layer1Id = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="Layer1")[0]

        # Mute/un-mute anon layer -> dirties scene
        with DirtySceneCheck(self):
            cmds.mayaUsdLayerEditor(layer1Id, edit=True, muteLayer=(True, shapePath))
        with DirtySceneCheck(self):
            cmds.mayaUsdLayerEditor(layer1Id, edit=True, muteLayer=(False, shapePath))

        # Remove anon layer -> dirties scene
        with DirtySceneCheck(self):
            cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, removeSubPath=(0, shapePath))

        # Reorder anon layer ->dirties scene
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, clear=True)
        layer1Id = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="Layer1")[0]
        layer2Id = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="Layer2")[0]
        layer3Id = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="Layer3")[0]
        self.assertIsNotNone(layer1Id)
        self.assertIsNotNone(layer2Id)
        self.assertIsNotNone(layer3Id)
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, removeSubPath=(2, shapePath))
        with DirtySceneCheck(self):
            cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, insertSubPath=(1, layer1Id))

    def testFileBackedLayers(self):
        mayaUtils.openTopLayerScene()
        shapePath, stage = getMayaStage()

        # Mute/Un-mute named layer -> dirties scene
        with DirtySceneCheck(self):
            stage.MuteLayer('Assembly_room_set.usda')
        with DirtySceneCheck(self):
            stage.UnmuteLayer('Assembly_room_set.usda')

        # Remove named layer -> dirties scene
        rootLayer = stage.GetRootLayer()
        with DirtySceneCheck(self):
            cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, removeSubPath=(0, shapePath))

    def testContextOps(self):
        mayaUtils.openTopLayerScene()

        # Ball_35 has both variants and references.
        ufePath = ufe.PathString.path('|transform1|proxyShape1,/Room_set/Props/Ball_35')
        item = ufe.Hierarchy.createItem(ufePath)
        contextOps = ufe.ContextOps.contextOps(item)

        # Change shading variant -> dirties scene
        with DirtySceneCheck(self):
            contextOps.doOp(['Variant Sets', 'shadingVariant', 'Cue'])

        # Unload reference -> dirties scene
        with DirtySceneCheck(self):
            contextOps.doOp(['Unload'])

        # Load reference -> dirties scene
        with DirtySceneCheck(self):
            contextOps.doOp(['Load'])

    def testDefaultPrim(self):
        mayaUtils.openTopLayerScene()
        shapePath, stage = getMayaStage()

        # Change default prim -> dirties scene
        prim = mayaUsd.ufe.ufePathToPrim('|transform1|proxyShape1,/Room_set/Props/Ball_35')
        with DirtySceneCheck(self):
            stage.SetDefaultPrim(prim)

    def testFlattenLayerStack(self):
        mayaUtils.openTopLayerScene()
        shapePath, stage = getMayaStage()

        # Flatten layer stack -> dirties scene
        rootLayer = stage.GetRootLayer()
        self.assertIsNotNone(rootLayer)
        flattenedLayer = stage.Flatten()
        self.assertIsNotNone(flattenedLayer)
        with DirtySceneCheck(self):
            rootLayer.TransferContent(flattenedLayer)

    def testMiscellaneousStage(self):
        shapePath, stage = getCleanMayaStage()
        rootLayer = stage.GetRootLayer()

        # Edit the start and end time code -> dirties scene
        with DirtySceneCheck(self):
            stage.SetStartTimeCode(stage.GetStartTimeCode() + 5.0)
        with DirtySceneCheck(self):
            stage.SetEndTimeCode(stage.GetEndTimeCode() - 5.0)

        # Set Metadata -> dirties scene
        with DirtySceneCheck(self):
            self.assertEqual(stage.GetMetadata('upAxis'), 'Y')
            stage.SetMetadata('upAxis', 'Z')
