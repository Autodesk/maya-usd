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
import tempfile
from os import path
from maya import cmds
import mayaUsd_createStageWithNewLayer
import mayaUsd
from pxr import Sdf, Usd

CLEAR = "-clear"
DISCARD = "-discardEdits"
PROXY_NODE_TYPE = "mayaUsdProxyShapeBase"

DUMMY_FILE_TEXT = \
    """#usda 1.0
def Sphere "ball"
    {
        custom string Winner = "shotFX"
    }
"""


def getCleanMayaStage():
    """ gets a stage that only has an anon layer """

    cmds.file(new=True, force=True)
    mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

    proxyShapes = cmds.ls(type=PROXY_NODE_TYPE, long=True)
    shapePath = proxyShapes[0]
    stage = mayaUsd.lib.GetPrim(shapePath).GetStage()
    return shapePath, stage


class MayaUsdLayerEditorCommandsTestCase(unittest.TestCase):
    """ test mel commands intended for the layer editor """

    @classmethod
    def setUpClass(cls):
        cmds.loadPlugin('mayaUsdPlugin')

    def testEditTarget(self):
        """ tests 'mayaUsdEditTarget' command, but also touches on adding anon layers """
        shapePath, stage = getCleanMayaStage()

        # original target is the root anonymous layer
        originalTargetID = stage.GetEditTarget().GetLayer().identifier
        rootLayer = stage.GetRootLayer()
        self.assertEqual(originalTargetID, rootLayer.identifier)

        currentTargetID = cmds.mayaUsdEditTarget(shapePath, query=True, editTarget=True)[0]
        self.assertEqual(originalTargetID, currentTargetID)

        # add two sub layers
        redLayerId = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="redLayer")[0]
        greenLayerId = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="greenLayer")[0]
        self.assertEqual(len(rootLayer.subLayerPaths), 2)

        cmds.mayaUsdEditTarget(shapePath, edit=True, editTarget=greenLayerId)
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, greenLayerId)
        # do we return that new target?
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier,
                         cmds.mayaUsdEditTarget(shapePath, query=True, editTarget=True)[0])
        undoStepTwo = greenLayerId

        cmds.mayaUsdEditTarget(shapePath, edit=True, editTarget=redLayerId)
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, redLayerId)
        undoStepOne = redLayerId

        # can we set the target back to the root?
        rootLayerID = rootLayer.identifier
        cmds.mayaUsdEditTarget(shapePath, edit=True, editTarget=rootLayerID)
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, rootLayerID)

        # undo
        cmds.undo()
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, undoStepOne)
        cmds.undo()
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, undoStepTwo)

        # test removing the edit target
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, greenLayerId)
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, removeSubPath=[0,shapePath])
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, rootLayerID)
        cmds.undo()
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, greenLayerId)

        cmds.mayaUsdEditTarget(shapePath, edit=True, editTarget=redLayerId)

        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, redLayerId)
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, removeSubPath=[1,shapePath])
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, rootLayerID)
        cmds.undo()
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, redLayerId)

        # test bad input
        with self.assertRaises(RuntimeError):
            cmds.mayaUsdEditTarget("bogusShape", query=True, editTarget=True)

        with self.assertRaises(RuntimeError):
            cmds.mayaUsdEditTarget("bogusShape", edit=True, editTarget=greenLayerId)

        with self.assertRaises(RuntimeError):
            cmds.mayaUsdEditTarget(shapePath, edit=True, editTarget="bogusLayer")

    def testSubLayerEditing(self):
        """ test 'mayaUsdLayerEditor' command """
        _shapePath, stage = getCleanMayaStage()
        rootLayer = stage.GetRootLayer()

        # -addAnonymous

        # add three layers. Note: these are always added at the top
        layer3Id = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="Layer1")[0]
        layer2Id = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="Layer2")[0]
        layer1Id = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="Layer3")[0]

        # check that they were added
        addedLayers = [layer1Id, layer2Id, layer3Id]
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)

        # test undo
        cmds.undo() # undo last add
        self.assertEqual(len(rootLayer.subLayerPaths), 2)
        cmds.undo()
        cmds.undo() # all gone now
        self.assertEqual(len(rootLayer.subLayerPaths), 0)
        # put them all back
        for _ in range(3):
            cmds.redo()
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)
        # remove them again
        for _ in range(3):
            cmds.undo()
        # redo  again
        for _ in range(3):
            cmds.redo()
        # all back?
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)

        # -removeSubPath
        # remove second sublayer
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, removeSubPath=[1,_shapePath])
        afterDeletion = [layer1Id, layer3Id]
        self.assertEqual(rootLayer.subLayerPaths, afterDeletion)

        # remove second sublayer again to leave only one
        afterDeletion = [layer1Id, layer3Id]
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, removeSubPath=[1,_shapePath])
        self.assertEqual(rootLayer.subLayerPaths, [layer1Id])

        # remove second sublayer  -- this time it's out of bounds
        with self.assertRaises(RuntimeError):
            cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, removeSubPath=[1,_shapePath])

        # undo twice to get back to three layers
        cmds.undo()
        cmds.undo()
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)
        # redo deletion of second layer
        cmds.redo()
        self.assertEqual(rootLayer.subLayerPaths, afterDeletion)
        cmds.undo()

        # put a sub layer and a sub-sub layer on the top layer.
        childLayerId = cmds.mayaUsdLayerEditor(layer1Id, edit=True, addAnonymous="ChildLayer")[0]
        grandChildLayerId = cmds.mayaUsdLayerEditor(childLayerId, edit=True, addAnonymous="GrandChild")[0]

        # delete the top layer.  See if it comes back after redo.
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, removeSubPath=[0,_shapePath])
        # check layer1 was deleted
        self.assertEqual(rootLayer.subLayerPaths, [layer2Id, layer3Id])
        # bring it back
        cmds.undo()
        firstLayer = Sdf.Layer.Find(layer1Id)
        self.assertIsNotNone(firstLayer)

        # check the children were not deleted
        self.assertEqual(firstLayer.subLayerPaths[0], childLayerId)
        childLayer = Sdf.Layer.Find(childLayerId)
        self.assertEqual(childLayer.subLayerPaths[0], grandChildLayerId)

        # -insertSubPath

        # insert two layers
        rootLayer.subLayerPaths.clear()
        second = "second.usda"
        first = "first.usda"
        newName = "david.usda"
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, insertSubPath=[0, second])
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, insertSubPath=[0, first])
        # message for anyone looking at the test log:
        # It's normal for stage.cpp to print the error that it can't find second.usa and first.usda, they do not exist"
        self.assertEqual(rootLayer.subLayerPaths, [first, second])

        # bad index
        with self.assertRaises(RuntimeError):
            cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, insertSubPath=[-2, "bogus"])
        with self.assertRaises(RuntimeError):
            cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, insertSubPath=[3, "bogus"])

        # -replaceSubPath

        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, replaceSubPath=[first, newName])
        self.assertEqual(rootLayer.subLayerPaths, [newName, second])
        # bad replace path
        with self.assertRaises(RuntimeError):
            cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, replaceSubPath=["notfound", newName])

        # -clear
        # -discardEdits

        testPasses = [CLEAR, DISCARD]
        for testPass in testPasses:

            if testPass == CLEAR:
                # get a clear stage
                rootLayer.subLayerPaths.clear()
            if testPass == DISCARD:
                # save and load a stage
                testUsdFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="dummy", delete=False, mode="w")
                testUsdFile.write(DUMMY_FILE_TEXT)
                testUsdFile.close()

                newStage = Usd.Stage.Open(testUsdFile.name)
                rootLayer = newStage.GetRootLayer()
                self.assertEqual(len(rootLayer.subLayerPaths), 0)

            childLayerId = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="Child")[0]
            grandChildLayerId = cmds.mayaUsdLayerEditor(childLayerId, edit=True, addAnonymous="GrandChild")[0]

            if testPass == CLEAR:
                cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, clear=True)
            if testPass == DISCARD:
                cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, discardEdits=True)

            # test everything is gone
            self.assertEqual(len(rootLayer.subLayerPaths), 0)

            cmds.undo()  # layers are back
            self.assertEqual(len(rootLayer.subLayerPaths), 1)
            self.assertEqual(rootLayer.subLayerPaths[0], childLayerId)
            cmds.redo()  # cleared/discarded again
            self.assertEqual(len(rootLayer.subLayerPaths), 0)
            cmds.undo()  # layers are back
            self.assertEqual(rootLayer.subLayerPaths[0], childLayerId)
            childLayer = Sdf.Layer.Find(childLayerId)
            self.assertEqual(childLayer.subLayerPaths[0], grandChildLayerId)

    def testRemoveEditTarget(self):

        shapePath, stage = getCleanMayaStage()
        rootLayer = stage.GetRootLayer()
        rootLayerID = rootLayer.identifier

        def setAndRemoveEditTargetRecursive(layer, index, editLayer):
            cmds.mayaUsdEditTarget(shapePath, edit=True, editTarget=editLayer)
            cmds.mayaUsdLayerEditor(layer.identifier, edit=True, removeSubPath=[index,shapePath])
            self.assertEqual(stage.GetEditTarget().GetLayer().identifier, rootLayerID)
            cmds.undo()
            self.assertEqual(stage.GetEditTarget().GetLayer().identifier, editLayer)

            subLayer = Sdf.Layer.FindRelativeToLayer(layer, editLayer)
            for subLayerOffset, subLayerId in enumerate(subLayer.subLayerPaths):
                setAndRemoveEditTargetRecursive(subLayer, subLayerOffset, subLayerId) 

        # build a layer hierarchy
        layerColorId = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="Color")[0]
        myLayerId = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="MyLayer")[0]

        layerRedId = cmds.mayaUsdLayerEditor(layerColorId, edit=True, addAnonymous="Red")[0]
        layerGreen = cmds.mayaUsdLayerEditor(layerColorId, edit=True, addAnonymous="Green")[0]
        layerBlue = cmds.mayaUsdLayerEditor(layerColorId, edit=True, addAnonymous="Blue")[0]

        layerDarkRed = cmds.mayaUsdLayerEditor(layerRedId, edit=True, addAnonymous="DarkRed")[0]
        layerLightRed = cmds.mayaUsdLayerEditor(layerRedId, edit=True, addAnonymous="LightRed")[0]

        mySubLayerId = cmds.mayaUsdLayerEditor(myLayerId, edit=True, addAnonymous="MySubLayer")[0]

        # traverse the layer tree
        # for each layer, set it as the edit target and remove it.
        for subLayerOffset, subLayerPath in enumerate(rootLayer.subLayerPaths):
            setAndRemoveEditTargetRecursive(rootLayer, subLayerOffset, subLayerPath)

        #
        # Test when the editTarget's parent (direct/ indirect) layer is removed
        #
        cmds.mayaUsdEditTarget(shapePath, edit=True, editTarget=layerDarkRed)
 
        # remove the Red layer (direct parent of DarkRed layer)
        cmds.mayaUsdLayerEditor(layerColorId, edit=True, removeSubPath=[2,shapePath])
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, rootLayerID)
        cmds.undo()
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, layerDarkRed)

        # remove the Color layer (indirect parent of DarkRed layer)
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, removeSubPath=[1,shapePath])
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, rootLayerID)
        cmds.undo()
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, layerDarkRed)

        #
        # Test with a layer that is a sublayer multiple times.
        #
        sharedLayerFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="sharedLayer", delete=False, mode="w")
        sharedLayerFile.write("#usda 1.0")
        sharedLayerFile.close()

        sharedLayer = path.normcase(sharedLayerFile.name)

        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, insertSubPath=[0, sharedLayer])
        cmds.mayaUsdLayerEditor(mySubLayerId, edit=True, insertSubPath=[0, sharedLayer])

        cmds.mayaUsdEditTarget(shapePath, edit=True, editTarget=sharedLayer)

        # remove the sharedLayer under the root layer.
        # the edit target should still be the shared layer
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, removeSubPath=[0,shapePath])
        self.assertEqual(path.normcase(stage.GetEditTarget().GetLayer().identifier), sharedLayer)
        cmds.undo()
        self.assertEqual(path.normcase(stage.GetEditTarget().GetLayer().identifier), sharedLayer)

        # remove the sharedLayer under the MySubLayer.
        # the edit target should still be the shared layer
        cmds.mayaUsdLayerEditor(mySubLayerId, edit=True, removeSubPath=[0,shapePath])
        self.assertEqual(path.normcase(stage.GetEditTarget().GetLayer().identifier), sharedLayer)
        cmds.undo()
        self.assertEqual(path.normcase(stage.GetEditTarget().GetLayer().identifier), sharedLayer)

        # remove MySubLayer (Direct parent of SharedLayer).
        # the edit target should still be the shared layer
        cmds.mayaUsdLayerEditor(myLayerId, edit=True, removeSubPath=[0,shapePath])
        self.assertEqual(path.normcase(stage.GetEditTarget().GetLayer().identifier), sharedLayer)
        cmds.undo()
        self.assertEqual(path.normcase(stage.GetEditTarget().GetLayer().identifier), sharedLayer)

        # remove MyLayer (Indirect parent of SharedLayer).
        # the edit target should still be the shared layer
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, removeSubPath=[0,shapePath])
        self.assertEqual(path.normcase(stage.GetEditTarget().GetLayer().identifier), sharedLayer)
        cmds.undo()
        self.assertEqual(path.normcase(stage.GetEditTarget().GetLayer().identifier), sharedLayer)

        # remove SharedLayer everywhere.
        # the edit target should become the root layer
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, removeSubPath=[0,shapePath])
        cmds.mayaUsdLayerEditor(mySubLayerId, edit=True, removeSubPath=[0,shapePath])
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, rootLayerID)
        cmds.undo()
        self.assertEqual(path.normcase(stage.GetEditTarget().GetLayer().identifier), sharedLayer)
