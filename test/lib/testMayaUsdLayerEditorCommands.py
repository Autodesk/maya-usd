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
import testUtils
import mayaUtils

from os import path, chmod
from stat import S_IREAD

from maya import cmds, mel
import mayaUsd_createStageWithNewLayer
import mayaUsd
from mayaUsd import ufe as mayaUsdUfe

from pxr import Sdf, Usd

CLEAR = "-clear"
DISCARD = "-discardEdits"

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
    shapePath = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
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

    def testRemoveMultipleLayers(self):

        shapePath, stage = getCleanMayaStage()
        rootLayer = stage.GetRootLayer()
        rootLayerID = rootLayer.identifier

        # Add six layers
        layerNames = []
        for index in range(1,7):
            layerName = "layer%d" % index
            layerName = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True,  addAnonymous=layerName)
            layerNames.append(layerName[0])

        # Verify that the root has the expected six layers.
        def subLayerNames():
            return list(rootLayer.subLayerPaths)

        def verifySubLayers(expectedNames):
            rootSubLayers = subLayerNames()
            self.assertEqual(len(expectedNames), len(rootSubLayers))
            for name in expectedNames:
                self.assertIn(name, rootSubLayers)

        verifySubLayers(layerNames)

        # Remove layers 0 and 1 and verify that the correct layers have been removed.
        #
        # Note: when inserted in USD, layers are added at the top, so layer 0 and 1 are
        #       layer5 and layer6.
        cmds.mayaUsdLayerEditor(rootLayer.identifier, removeSubPath=((0, shapePath), (1, shapePath)))
        verifySubLayers(layerNames[0:4])

    def testMoveSubPath(self):
        """ test 'mayaUsdLayerEditor' command 'moveSubPath' paramater """

        def moveSubPath(parentLayer, subPath, newParentLayer, index, originalSubLayerPaths, newSubLayerPaths):
            cmds.mayaUsdLayerEditor(parentLayer.identifier, edit=True, moveSubPath=[subPath, newParentLayer.identifier, index])
            self.assertEqual(newParentLayer.subLayerPaths, newSubLayerPaths)

            cmds.undo()
            self.assertEqual(newParentLayer.subLayerPaths, originalSubLayerPaths)

            cmds.redo()
            self.assertEqual(newParentLayer.subLayerPaths, newSubLayerPaths)

            cmds.undo()
            self.assertEqual(newParentLayer.subLayerPaths, originalSubLayerPaths)

        def moveElement(list, item, index):
            l = list[:] #copy the list
            l.remove(item)
            l.insert(index, item)
            return l

        shapePath, stage = getCleanMayaStage()
        rootLayer = stage.GetRootLayer()

        layerId1 = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="MyLayer1")[0]
        layerId2 = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="MyLayer2")[0]
        layerId3 = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="MyLayer3")[0]

        originalSubLayerPaths = [layerId3, layerId2, layerId1]
        self.assertEqual(rootLayer.subLayerPaths, originalSubLayerPaths)

        # Test moving each layers under the root layer to any valid index in the root layer
        # and test out-of-bound index
        for layer in originalSubLayerPaths:
            for i in range(len(originalSubLayerPaths)):
                expectedSubPath = moveElement(originalSubLayerPaths, layer, i)
                moveSubPath(rootLayer, layer, rootLayer, i, originalSubLayerPaths, expectedSubPath)

                with self.assertRaises(RuntimeError):
                    cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, moveSubPath=[layer, rootLayer.identifier, len(originalSubLayerPaths)])
                self.assertEqual(rootLayer.subLayerPaths, originalSubLayerPaths)

        #
        # Test moving sublayer (layer2 and layer3) inside a new parent layer (layer1)
        #

        layer1 = Sdf.Layer.Find(layerId1)     
        self.assertTrue(len(layer1.subLayerPaths) == 0)

        # layer1 has no subLayer so index 1 is invalide
        with self.assertRaises(RuntimeError):
            cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, moveSubPath=[layerId3, layerId1, 1])
        self.assertTrue(len(layer1.subLayerPaths) == 0)
        self.assertEqual(rootLayer.subLayerPaths, originalSubLayerPaths)

        # Move layer3 from the root layer inside layer1 at index 0
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, moveSubPath=[layerId3, layerId1, 0])
        self.assertEqual(rootLayer.subLayerPaths, [layerId2, layerId1])
        self.assertEqual(layer1.subLayerPaths, [layerId3])

        # Move layer2 from the root layer inside layer1 at index 0
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, moveSubPath=[layerId2, layerId1, 0])
        self.assertEqual(rootLayer.subLayerPaths, [layerId1])
        self.assertEqual(layer1.subLayerPaths, [layerId2, layerId3])

        cmds.undo()
        self.assertEqual(rootLayer.subLayerPaths, [layerId2, layerId1])
        self.assertEqual(layer1.subLayerPaths, [layerId3])

        # Move layer2 from the root layer inside layer1 at index 1
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, moveSubPath=[layerId2, layerId1, 1])
        self.assertEqual(rootLayer.subLayerPaths, [layerId1])
        self.assertEqual(layer1.subLayerPaths, [layerId3, layerId2])

        cmds.undo()
        self.assertEqual(rootLayer.subLayerPaths, [layerId2, layerId1])
        self.assertEqual(layer1.subLayerPaths, [layerId3])

        # Undo moving layer3 inside layer1
        cmds.undo()
        self.assertEqual(rootLayer.subLayerPaths, [layerId3, layerId2, layerId1])
        self.assertEqual(layer1.subLayerPaths, [])

        # Insert layer3 inside layer1 and try to move the layer3 from the root layer
        # inside layer1. This should fail.
        cmds.mayaUsdLayerEditor(layerId1, edit=True, insertSubPath=[0, layerId3])
        self.assertEqual(layer1.subLayerPaths, [layerId3])

        with self.assertRaises(RuntimeError):
            cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, moveSubPath=[layerId3, layerId1, 0])
        self.assertEqual(rootLayer.subLayerPaths, originalSubLayerPaths)
        self.assertEqual(layer1.subLayerPaths, [layerId3])

    def testMoveRelativeSubPath(self):
        """ test 'mayaUsdLayerEditor' command 'moveSubPath' paramater for relative subpaths """

        shapePath, stage = getCleanMayaStage()
        rootLayer = stage.GetRootLayer()

        # Create a temporary directory in the current diretory
        myDir = testUtils.TemporaryDirectory(suffix="testMoveRelativeSubPathDir")
        myDir_path, myDir_name = path.split(myDir.name)

        # Create a new usda file in the current diretory and add it to the root layer through absolute path
        absLayer1File = tempfile.NamedTemporaryFile(suffix=".usda", prefix="absLayer1", dir=myDir_path, delete=False, mode="w")
        absLayer1File.write("#usda 1.0")
        absLayer1File.close()
        absLayer1Id = path.normpath(absLayer1File.name)
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, insertSubPath=[0, absLayer1Id])
        absLayer1 = Sdf.Layer.Find(absLayer1Id)

        # Create a new usda file in the new temporary diretory and add it to the root layer through absolute path
        absLayer2File = tempfile.NamedTemporaryFile(suffix=".usda", prefix="absLayer2", dir=myDir.name, delete=False, mode="w")
        absLayer2File.write("#usda 1.0")
        absLayer2File.close()
        absLayer2Id = path.normpath(absLayer2File.name)
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, insertSubPath=[1, absLayer2Id])
        absLayer2 = Sdf.Layer.Find(absLayer2Id)

        # Create a new usda file in the new temporary diretory and add it to absLayer2 through its relative path
        relLayerFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="relLayer", dir=myDir.name, delete=False, mode="w")
        relLayerFile.write("#usda 1.0")
        relLayerFile.close()
        relLayerFileId = path.basename(relLayerFile.name) # relative path
        cmds.mayaUsdLayerEditor(absLayer2.identifier, edit=True, insertSubPath=[0, relLayerFileId])

        # Now move the relative sublayer from absLayer2 to absLayer1
        cmds.mayaUsdLayerEditor(absLayer2.identifier, edit=True, moveSubPath=[relLayerFileId, absLayer1.identifier, 0])

        # The relative sublayer's path should change now to include the directory name
        relLayerNewFileId = myDir_name + "/" + relLayerFileId
        self.assertTrue(len(absLayer1.subLayerPaths) == 1)
        self.assertEqual(absLayer1.subLayerPaths[0], relLayerNewFileId)

    def testLockLayer(self):
        """ test 'mayaUsdLayerEditor' command 'lockLayer' parameter """
        
        # Helpers
        def createLayer():
            layer = Sdf.Layer.CreateAnonymous()
            stage.GetRootLayer().subLayerPaths.append(layer.identifier)
            return layer
        
        shapePath, stage = getCleanMayaStage()
        self.assertTrue(stage)
        
        subLayer = createLayer()
        self.assertTrue(subLayer.permissionToEdit)
        
        # Locking a layer
        cmds.mayaUsdLayerEditor(subLayer.identifier, edit=True, lockLayer=(1, 0, shapePath))
        self.assertFalse(subLayer.permissionToEdit)
        cmds.undo()
        self.assertTrue(subLayer.permissionToEdit)
        cmds.redo()
        self.assertFalse(subLayer.permissionToEdit)
        # Unlocking a layer
        cmds.mayaUsdLayerEditor(subLayer.identifier, edit=True, lockLayer=(0, 0, shapePath))
        self.assertTrue(subLayer.permissionToEdit)
        # System locking a layer
        cmds.mayaUsdLayerEditor(subLayer.identifier, edit=True, lockLayer=(2, 0, shapePath))
        self.assertFalse(subLayer.permissionToEdit)
        self.assertFalse(subLayer.permissionToSave)
        cmds.undo()
        self.assertTrue(subLayer.permissionToEdit)

    def testRecursiveLockSingleLayer(self):
        """
        Test the 'mayaUsdLayerEditor' command 'lockLayer' parameter in recursive mode
        when the layer has no sub0layer
        """
        
        rootLayerPath = testUtils.getTestScene("layerLocking", "layerLocking.usda")
        stage = Usd.Stage.Open(rootLayerPath)
        topLayer = stage.GetRootLayer();
        subLayer1 = Sdf.Layer.FindRelativeToLayer(topLayer, topLayer.subLayerPaths[0])
        subLayer1_1 = Sdf.Layer.FindRelativeToLayer(subLayer1, subLayer1.subLayerPaths[0])

        layerLockingShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        shapePath = layerLockingShapes[0]

        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        
        # Locking a layer recursively
        cmds.mayaUsdLayerEditor(subLayer1_1.identifier, edit=True, lockLayer=(1, 1, shapePath))
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        cmds.undo()
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        cmds.redo()
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
    
        # Unlocking a layer recursively
        cmds.mayaUsdLayerEditor(subLayer1_1.identifier, edit=True, lockLayer=(0, 1, shapePath))
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        cmds.undo()
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        cmds.redo()
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)

        # System locking a layer recursively
        cmds.mayaUsdLayerEditor(subLayer1_1.identifier, edit=True, lockLayer=(2, 1, shapePath))
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        cmds.undo()
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        cmds.redo()
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)

        # Unlocking a system-locked layer recursively
        cmds.mayaUsdLayerEditor(subLayer1_1.identifier, edit=True, lockLayer=(0, 1, shapePath))
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        cmds.undo()
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        cmds.redo()
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)

    def testRecursiveLockMultiLayers(self):
        """
        Test the 'mayaUsdLayerEditor' command 'lockLayer' parameter in recursive mode
        when it has a sub-layer
        """
        
        rootLayerPath = testUtils.getTestScene("layerLocking", "layerLocking.usda")
        stage = Usd.Stage.Open(rootLayerPath)
        topLayer = stage.GetRootLayer();
        subLayer1 = Sdf.Layer.FindRelativeToLayer(topLayer, topLayer.subLayerPaths[0])
        subLayer1_1 = Sdf.Layer.FindRelativeToLayer(subLayer1, subLayer1.subLayerPaths[0])

        layerLockingShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        shapePath = layerLockingShapes[0]

        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        
        # Locking a layer recursively
        cmds.mayaUsdLayerEditor(subLayer1.identifier, edit=True, lockLayer=(1, 1, shapePath))
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        cmds.undo()
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        cmds.redo()
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
    
        # Unlocking a layer recursively
        cmds.mayaUsdLayerEditor(subLayer1.identifier, edit=True, lockLayer=(0, 1, shapePath))
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        cmds.undo()
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        cmds.redo()
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)

        # System locking a layer
        cmds.mayaUsdLayerEditor(subLayer1.identifier, edit=True, lockLayer=(2, 1, shapePath))
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertFalse(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        cmds.undo()
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        cmds.redo()
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertFalse(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)

        # Unlocking a system-locked layer recursively
        #
        # Note: we use the flag to skip system-locked layer to *only* unlock
        #       the layer itself because by design we don't want to recursively
        #       unlock system-locked layers from the UI.
        #
        #       Otherwise, unlocking recursively inthe UI would unlock system
        #       layers, which is not something we want the user to do.
        cmds.mayaUsdLayerEditor(subLayer1.identifier, edit=True, skipSystemLocked=True, lockLayer=(0, 1, shapePath))
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        cmds.undo()
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertFalse(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        cmds.redo()
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        cmds.undo()

        # Unlocking a system-locked layer recursively
        cmds.mayaUsdLayerEditor(subLayer1.identifier, edit=True, lockLayer=(0, 1, shapePath))
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        cmds.undo()
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertFalse(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        cmds.redo()
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)

    def testRefreshSystemLock(self):
        # FileBacked Layer Write Permission
        # 1- Loading the test scene
        rootLayerPath = testUtils.getTestScene("layerLocking", "layerLocking.usda")
        stage = Usd.Stage.Open(rootLayerPath)
        topLayer = stage.GetRootLayer();
        layerLockingShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = layerLockingShapes[0]
        # 2- Setting a system lock on a layer loaded from a file
        cmds.mayaUsdLayerEditor(topLayer.identifier, edit=True, lockLayer=(2, 0, proxyShapePath))
        self.assertFalse(topLayer.permissionToEdit)
        self.assertFalse(topLayer.permissionToSave)
        # 3- Refreshing the system lock should remove the lock if the file is writable
        cmds.mayaUsdLayerEditor(topLayer.identifier, edit=True, refreshSystemLock=(proxyShapePath, 1))
        self.assertTrue(topLayer.permissionToEdit)
        self.assertTrue(topLayer.permissionToSave)
        
    def testLockLayerAndSubLayers(self):
        # Locking a layer and its sublayer
        # 1- Loading the test scene
        rootLayerPath = testUtils.getTestScene("layerLocking", "layerLocking.usda")
        stage = Usd.Stage.Open(rootLayerPath)
        topLayer = stage.GetRootLayer();
        layerLockingShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = layerLockingShapes[0]
        # 2- Setting a lock on the top layer with the option to lock its sublayer
        cmds.mayaUsdLayerEditor(topLayer.identifier, edit=True, lockLayer=(1, 1, proxyShapePath))
        self.assertFalse(topLayer.permissionToEdit)
        # 3- Check that sublayer is also locked
        subLayer = Sdf.Layer.FindRelativeToLayer(topLayer, topLayer.subLayerPaths[0])
        self.assertFalse(subLayer.permissionToEdit)
        # 4- Checking that no layers are modifiable
        self.assertFalse(mayaUsdUfe.isAnyLayerModifiable(stage))
        # 5- Undo and check that both top and sublayer are unlocked
        cmds.undo()
        self.assertTrue(topLayer.permissionToEdit)
        self.assertTrue(subLayer.permissionToEdit)
        # 6- Checking that at least one layer is modifiable
        self.assertTrue(mayaUsdUfe.isAnyLayerModifiable(stage))
    
    def testRefreshSystemLockCallback(self):
        # FileBacked Layer Write Permission
        # 1- Loading the test scene
        rootLayerPath = testUtils.getTestScene("layerLocking", "layerLocking.usda")
        stage = Usd.Stage.Open(rootLayerPath)
        topLayer = stage.GetRootLayer();
        subLayer = Sdf.Layer.FindRelativeToLayer(topLayer, topLayer.subLayerPaths[0])
        layerLockingShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = layerLockingShapes[0]
        # 2- Setting a system lock on a layer loaded from a file and its sub-layer
        cmds.mayaUsdLayerEditor(topLayer.identifier, edit=True, lockLayer=(2, 0, proxyShapePath))
        self.assertFalse(topLayer.permissionToEdit)
        self.assertFalse(topLayer.permissionToSave)
        # 3- Attach callbacks to capture any system-lock changes due to refreshSystemLock
        self.callCount = 0
        def refreshSystemLockCallback(context, callbackData):
            layerIds = callbackData.get('affectedLayerIds')
            # Check that only the top layers is affected
            self.assertTrue(len(layerIds), 1)
            self.assertEqual(layerIds[0], topLayer.identifier)
            # Check that the top layer is unlocked due to the refresh
            self.assertTrue(topLayer.permissionToEdit)
            self.assertTrue(topLayer.permissionToSave)
            # Check that the sublayer is unchanged
            self.assertTrue(subLayer.permissionToEdit)
            self.assertTrue(subLayer.permissionToSave)
            self.callCount = self.callCount + 1
        
        from usdUfe import registerUICallback, unregisterUICallback
        mayaUsd.lib.registerUICallback('onRefreshSystemLock', refreshSystemLockCallback)        
        # 4- Refreshing the system lock should remove the lock but also get re-locked by refreshSystemLockCallback
        cmds.mayaUsdLayerEditor(topLayer.identifier, edit=True, refreshSystemLock=(proxyShapePath, 1))
        self.assertEqual(self.callCount, 1)

        # 5- Unregistering the callback and refreshing the system lock should not call
        #    call the callback again.
        #
        # Note: we must relock the layer for the callback to be called, otherwise it does not get
        #       called as the status of the layer would not have changed during the refresh.
        cmds.mayaUsdLayerEditor(topLayer.identifier, edit=True, lockLayer=(2, 0, proxyShapePath))
        mayaUsd.lib.unregisterUICallback('onRefreshSystemLock', refreshSystemLockCallback)        
        cmds.mayaUsdLayerEditor(topLayer.identifier, edit=True, refreshSystemLock=(proxyShapePath, 1))
        self.assertEqual(self.callCount, 1)

        # 6- Unregistering again should do nothing and not crash.
        mayaUsd.lib.unregisterUICallback('onRefreshSystemLock', refreshSystemLockCallback)        

    def _verifyStageAfterRefreshSystemLock(
            self, writableFiles, expectedLayerModifiable, callback=None):

        with testUtils.TemporaryDirectory(prefix='RefreshLock') as testDir:
            # Create a stage with a simple layer stack.
            rootLayerPath = path.join(testDir, "root.usda")
            rootLayer = Sdf.Layer.CreateNew(rootLayerPath)

            subLayerPath = path.join(testDir, "sub.usda")
            subLayer = Sdf.Layer.CreateNew(subLayerPath)

            rootLayer.subLayerPaths.append(subLayer.identifier)
            rootLayer.Save()

            proxyShape, stage = mayaUtils.createProxyFromFile(rootLayerPath)

            # Apply requested file permissions if needed.
            if not writableFiles:
                for layer in stage.GetLayerStack(False):
                    chmod(layer.realPath, S_IREAD)

            if callback is not None:
                # Install the given callback.
                mayaUsd.lib.registerUICallback('onRefreshSystemLock', callback)

                # Alter a layer lock to ensure the callback is triggered on
                # refreshSystemLock.
                lockStatus = 0 if mayaUsd.lib.isLayerSystemLocked(rootLayer) else 2
                cmds.mayaUsdLayerEditor(rootLayerPath, e=True,
                                        lockLayer=(lockStatus, 0, proxyShape))

            cmds.mayaUsdLayerEditor(rootLayerPath, e=True,
                                    refreshSystemLock=(proxyShape, 1))

            if callback is not None:
                mayaUsd.lib.unregisterUICallback('onRefreshSystemLock', callback)

            # Verify that the expected locks were applied
            # e.g. during the callback.
            self.assertEqual(mayaUsdUfe.isAnyLayerModifiable(stage),
                             expectedLayerModifiable)

            # Verify that refreshSystemLock properly handled the editTarget
            # e.g. that it accounts for lock changes during the callback.
            if expectedLayerModifiable:
                # The initial target should be preserved in this case.
                expectedTargetLayer = rootLayer
                self.assertEqual(stage.GetEditTarget().GetLayer(), expectedTargetLayer)
            else:
                # Edit target should have been forced to session layer 
                # or unchanged if the current targeted layer is modifiable.
                if mayaUsd.lib.isLayerLocked(stage.GetEditTarget().GetLayer()):
                    expectedTargetLayer = stage.GetSessionLayer()
                    self.assertEqual(stage.GetEditTarget().GetLayer(), expectedTargetLayer)
                else:
                    self.assertEqual(mayaUsd.lib.isLayerLocked(stage.GetEditTarget().GetLayer()), False)
                
    def testRefreshSystemLockWithoutCallback(self):
        """
        Test refreshSystemLocks without any callback.
        """
        self._verifyStageAfterRefreshSystemLock(
            writableFiles=False, expectedLayerModifiable=False)

        self._verifyStageAfterRefreshSystemLock(
            writableFiles=True, expectedLayerModifiable=True)

    def testRefreshSystemLockCallbackLockingAll(self):
        """
        Test refreshSystemLocks with a callback that force a systemLock on all
        layers even if the usd files are writable.
        """
        def callback(context, callbackData):
            shapePath = context.get('proxyShapePath')
            stage = mayaUsdUfe.getStage(shapePath)
            for layer in stage.GetLayerStack(False):
                mayaUsd.lib.systemLockLayer(shapePath, layer)

        self._verifyStageAfterRefreshSystemLock(
            writableFiles=True, expectedLayerModifiable=False,
            callback=callback)

    def testRefreshSystemLockWithCallbackUnlockingAll(self):
        """
        Test refreshSystemLocks with a callback that will unlock all
        layers while they were automatically locked according to usd files
        permissions.
        """
        def callback(context, callbackData):
            shapePath = context.get('proxyShapePath')
            for layerId in callbackData.get('affectedLayerIds'):
                mayaUsd.lib.unlockLayer(shapePath, Sdf.Find(layerId))

        self._verifyStageAfterRefreshSystemLock(
            writableFiles=False, expectedLayerModifiable=True,
            callback=callback)

    def testRefreshSystemLockWithCallbackUnlockingEditTarget(self):
        """
        Test refreshSystemLocks with a callback that unlocks the current
        edit target layer while it was automatically locked according
        to usd file permission.
        """
        def callback(context, callbackData):
            shapePath = context.get('proxyShapePath')
            stage = mayaUsdUfe.getStage(shapePath)
            mayaUsd.lib.unlockLayer(shapePath, stage.GetEditTarget().GetLayer())

        self._verifyStageAfterRefreshSystemLock(
            writableFiles=False, expectedLayerModifiable=True,
            callback=callback)

    def testMuteLayer(self):
        """ test 'mayaUsdLayerEditor' command 'muteLayer' paramater """

        def testMuteLayerImpl(addLayerFunc):

            def checkMuted(layer, stage):
                # Make sure the layer is muted inside the stage.
                self.assertTrue(stage.IsLayerMuted(layer.identifier))
                self.assertTrue(layer.identifier in stage.GetMutedLayers())
                # Make sure the stage does not used the muted layer
                self.assertFalse(layer in stage.GetLayerStack(False))
                self.assertFalse(layer in stage.GetUsedLayers(False))

            def checkUnMuted(layer, stage):
                self.assertFalse(stage.IsLayerMuted(layer.identifier))
                self.assertFalse(layer.identifier in stage.GetMutedLayers())
                self.assertTrue(layer in stage.GetLayerStack(False))
                self.assertTrue(layer in stage.GetUsedLayers(False))

            shapePath, stage = getCleanMayaStage()
            rootLayer = stage.GetRootLayer()

            layer = addLayerFunc(rootLayer)
            
            checkUnMuted(layer, stage)
            
            # Mute the layer
            cmds.mayaUsdLayerEditor(layer.identifier, edit=True, muteLayer=(True, shapePath))
            checkMuted(layer, stage)
        
            # undo mute
            cmds.undo()
            checkUnMuted(layer, stage)

            # redo mute
            cmds.redo()
            checkMuted(layer, stage)
        
        # Add an anonymous layer under the "parentLayer" 
        def addAnonymousLayer(parentLayer):
            layerId = cmds.mayaUsdLayerEditor(parentLayer.identifier, edit=True, addAnonymous="MyLayer")[0]
            layer = Sdf.Layer.Find(layerId)
            return layer

        # Add a layer baked file under the "parentLayer"
        # The layer is added by using it's identifier.
        def addFileBakedLayer(parentLayer):
            tempFile = tempfile.NamedTemporaryFile(suffix=".usda", delete=False, mode="w")
            tempFile.write("#usda 1.0")
            tempFile.close()
            layer = Sdf.Layer.FindOrOpen(tempFile.name)
            cmds.mayaUsdLayerEditor(parentLayer.identifier, edit=True, insertSubPath=[0, layer.identifier])
            self.assertTrue(layer)
            return layer

        # Add a layer baked file under the "parentLayer"
        # The layer is added by using it's filesystem path.
        def addFileBakedLayerByPath(parentLayer):
            tempFile = tempfile.NamedTemporaryFile(suffix=".usda", delete=False, mode="w")
            tempFile.write("#usda 1.0")
            tempFile.close()
            cmds.mayaUsdLayerEditor(parentLayer.identifier, edit=True, insertSubPath=[0, tempFile.name])
            layer = Sdf.Layer.FindOrOpen(tempFile.name)
            self.assertTrue(layer)
            return layer

        testMuteLayerImpl(addAnonymousLayer)
        testMuteLayerImpl(addFileBakedLayer)
        testMuteLayerImpl(addFileBakedLayerByPath)

    def testPathRelativeToMayaSceneFile(self):
        # Function mayaUsdLib.Util.getPathRelativeToMayaSceneFile is used during 
        # USD root layer saving/loading, and since we cannot test the full workflow
        # as it opens a file dialog and requires user input, we make sure to test 
        # the function itself here.
        
        ballFilePath = path.normpath(testUtils.getTestScene('ballset', 'StandaloneScene', 'top_layer.usda')).replace('\\', '/')

        # Without Maya scene file, the absolute path is returned
        cmds.file(newFile=True, force=True)
        filePathAbs = mayaUsd.lib.Util.getPathRelativeToMayaSceneFile(ballFilePath)
        self.assertEqual(filePathAbs, ballFilePath)

        # With Maya scene file, the relative path is returned
        mayaSceneFilePath = testUtils.getTestScene("ballset", "StandaloneScene", "top_layer.ma")
        cmds.file(mayaSceneFilePath, force=True, open=True)
        filePathRel = mayaUsd.lib.Util.getPathRelativeToMayaSceneFile(ballFilePath)
        self.assertEqual(filePathRel, 'top_layer.usda')
        
    def testMergeWithSublayers(self):
        """ test 'mayaUsdLayerEditor' command 'flatten' parameter to merge a layer with its sublayers """
        shapePath, stage = getCleanMayaStage()
        rootLayer = stage.GetRootLayer()

        # Create two separate layer hierarchies under root
        # Root
        #   Layer1
        #     Layer1Sub
        #   Layer2
        #     Layer2Sub
        layer1Id = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="Layer1")[0]
        layer2Id = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="Layer2")[0]
        layer1SubId = cmds.mayaUsdLayerEditor(layer1Id, edit=True, addAnonymous="Layer1Sub")[0]
        layer2SubId = cmds.mayaUsdLayerEditor(layer2Id, edit=True, addAnonymous="Layer2Sub")[0]

        layer1 = Sdf.Layer.Find(layer1Id)
        layer2 = Sdf.Layer.Find(layer2Id)
        layer1Sub = Sdf.Layer.Find(layer1SubId)
        layer2Sub = Sdf.Layer.Find(layer2SubId)

        # Ball1 defined in Layer1Sub, transformed in Layer1
        ball1Spec = Sdf.PrimSpec(layer1Sub, "Ball1", Sdf.SpecifierDef, "Sphere")
        ball1OverSpec = Sdf.PrimSpec(layer1, "Ball1", Sdf.SpecifierOver)
        ball1Attr = Sdf.AttributeSpec(ball1OverSpec, "radius", Sdf.ValueTypeNames.Double)
        ball1Attr.default = 5.0

        # Ball2 defined in Layer2Sub, transformed in Layer2
        ball2Spec = Sdf.PrimSpec(layer2Sub, "Ball2", Sdf.SpecifierDef, "Sphere")
        ball2OverSpec = Sdf.PrimSpec(layer2, "Ball2", Sdf.SpecifierOver)
        ball2Attr = Sdf.AttributeSpec(ball2OverSpec, "radius", Sdf.ValueTypeNames.Double)
        ball2Attr.default = 10.0

        # Verify initial structure
        self.assertEqual(len(layer1.subLayerPaths), 1)
        self.assertEqual(len(layer2.subLayerPaths), 1)
        self.assertEqual(len(rootLayer.subLayerPaths), 2)

        # Flatten Layer1 (merge Layer1Sub into Layer1)
        cmds.mayaUsdLayerEditor(layer1Id, edit=True, flatten=True)
        self.assertEqual(len(layer1.subLayerPaths), 0)
        self.assertIsNotNone(layer1.GetPrimAtPath("/Ball1"))

        # Flatten Layer2 (merge Layer2Sub into Layer2)
        cmds.mayaUsdLayerEditor(layer2Id, edit=True, flatten=True)
        self.assertEqual(len(layer2.subLayerPaths), 0)
        self.assertIsNotNone(layer2.GetPrimAtPath("/Ball2"))

        # Flatten Root (merge flattened Layer1 and Layer2 into Root)
        self.assertEqual(len(rootLayer.subLayerPaths), 2)
        cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, flatten=True)
        self.assertEqual(len(rootLayer.subLayerPaths), 0)

        # Verify all content is now in root layer
        self.assertIsNotNone(rootLayer.GetPrimAtPath("/Ball1"))
        self.assertIsNotNone(rootLayer.GetPrimAtPath("/Ball2"))

        # Undo root flatten
        cmds.undo()
        self.assertEqual(len(rootLayer.subLayerPaths), 2)

        # Undo Layer2 flatten
        cmds.undo()
        self.assertEqual(len(layer2.subLayerPaths), 1)

        # Undo Layer1 flatten
        cmds.undo()
        self.assertEqual(len(layer1.subLayerPaths), 1)

        # Redo Layer1 flatten
        cmds.redo()
        self.assertEqual(len(layer1.subLayerPaths), 0)

        # Redo Layer2 flatten
        cmds.redo()
        self.assertEqual(len(layer2.subLayerPaths), 0)

        # Redo Root flatten
        cmds.redo()
        self.assertEqual(len(rootLayer.subLayerPaths), 0)

    def testMergeWithSublayersWithDirtySavedLayers(self):
        """
        Test that merge with sublayers and undoing preserves:
        1. Anonymous layers and their content
        2. Clean saved layers and their content (not dirty after undo)
        3. Dirty saved layers with their in-memory changes (dirty after undo)
        4. Root layer which we call flatten on preserves its clean state (not dirty after undo).
        """
        with testUtils.TemporaryDirectory(prefix='FlattenDirty') as testDir:
            # Create a file-backed root layer (instead of anonymous) so we can save it.
            rootLayerPath = path.join(testDir, "rootLayer.usda")
            rootLayer = Sdf.Layer.CreateNew(rootLayerPath)
            rootLayer.Save()

            proxyShape, stage = mayaUtils.createProxyFromFile(rootLayerPath)
            shapePath = proxyShape
            rootLayer = stage.GetRootLayer()

            # Create three sublayers with different states:
            # 1. Anonymous layer.
            anonLayerId = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="AnonLayer")[0]
            anonLayer = Sdf.Layer.Find(anonLayerId)

            # 2. Clean saved layer (file-backed, not modified).
            cleanLayerPath = path.join(testDir, "cleanLayer.usda")
            cleanLayer = Sdf.Layer.CreateNew(cleanLayerPath)
            cleanLayer.Save()
            cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, insertSubPath=[0, cleanLayerPath])
            cleanLayer = Sdf.Layer.FindOrOpen(cleanLayerPath)

            # 3. Dirty saved layer (file-backed, modified in memory).
            dirtyLayerPath = path.join(testDir, "dirtyLayer.usda")
            dirtyLayer = Sdf.Layer.CreateNew(dirtyLayerPath)
            dirtyLayer.Save()
            cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, insertSubPath=[0, dirtyLayerPath])
            dirtyLayer = Sdf.Layer.FindOrOpen(dirtyLayerPath)

            # Add content to each layer.
            # Anonymous layer: Ball1 with radius 5.
            ball1Spec = Sdf.PrimSpec(anonLayer, "Ball1", Sdf.SpecifierDef, "Sphere")
            ball1Attr = Sdf.AttributeSpec(ball1Spec, "radius", Sdf.ValueTypeNames.Double)
            ball1Attr.default = 5.0

            # Clean layer: Ball2 with radius 10 (saved to disk).
            ball2Spec = Sdf.PrimSpec(cleanLayer, "Ball2", Sdf.SpecifierDef, "Sphere")
            ball2Attr = Sdf.AttributeSpec(ball2Spec, "radius", Sdf.ValueTypeNames.Double)
            ball2Attr.default = 10.0
            cleanLayer.Save()  # Save to disk.

            # Dirty layer: Ball3 with radius 15 (saved to disk first).
            ball3Spec = Sdf.PrimSpec(dirtyLayer, "Ball3", Sdf.SpecifierDef, "Sphere")
            ball3Attr = Sdf.AttributeSpec(ball3Spec, "radius", Sdf.ValueTypeNames.Double)
            ball3Attr.default = 15.0
            dirtyLayer.Save()

            # In-memory change to make the layer dirty.
            ball3Attr.default = 20.0

            # Verify initial state
            self.assertEqual(len(rootLayer.subLayerPaths), 3)
            self.assertFalse(cleanLayer.dirty)
            self.assertTrue(dirtyLayer.dirty)

            # Save root layer so it's clean before flatten.
            rootLayer.Save()
            self.assertFalse(rootLayer.dirty, "Root layer should be clean after save")

            # Verify the in-memory value for dirty layer is 20, not the disk value of 15.
            self.assertEqual(dirtyLayer.GetPrimAtPath("/Ball3").attributes["radius"].default, 20.0)

            # Flatten
            cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, flatten=True)

            # Verify flatten worked
            self.assertEqual(len(rootLayer.subLayerPaths), 0)
            self.assertIsNotNone(rootLayer.GetPrimAtPath("/Ball1"))
            self.assertIsNotNone(rootLayer.GetPrimAtPath("/Ball2"))
            self.assertIsNotNone(rootLayer.GetPrimAtPath("/Ball3"))
            # Ball3 should have the in-memory value 20.0 not the disk value 15.0.
            self.assertEqual(rootLayer.GetPrimAtPath("/Ball3").attributes["radius"].default, 20.0)

            cmds.undo()

            # Verify structure is restored.
            self.assertEqual(len(rootLayer.subLayerPaths), 3)

            # Re-fetch layers after undo.
            anonLayer = Sdf.Layer.Find(anonLayerId)
            cleanLayer = Sdf.Layer.FindOrOpen(cleanLayerPath)
            dirtyLayer = Sdf.Layer.FindOrOpen(dirtyLayerPath)

            # Verify all layers still exist.
            self.assertIsNotNone(anonLayer)
            self.assertIsNotNone(cleanLayer)
            self.assertIsNotNone(dirtyLayer)

            # Verify content is restored in each layer.
            self.assertIsNotNone(anonLayer.GetPrimAtPath("/Ball1"))
            self.assertEqual(anonLayer.GetPrimAtPath("/Ball1").attributes["radius"].default, 5.0)
            self.assertIsNotNone(cleanLayer.GetPrimAtPath("/Ball2"))
            self.assertEqual(cleanLayer.GetPrimAtPath("/Ball2").attributes["radius"].default, 10.0)
            self.assertIsNotNone(dirtyLayer.GetPrimAtPath("/Ball3"))

            # Dirty layer should have the in-memory value 20.0, not disk value.
            self.assertEqual(dirtyLayer.GetPrimAtPath("/Ball3").attributes["radius"].default, 20.0,
                           "Dirty layer should restore with in-memory changes, not clean disk version")

            # Verify dirty state is preserved for all layers.
            self.assertFalse(cleanLayer.dirty, "Clean layer should remain clean after undo")
            self.assertTrue(dirtyLayer.dirty, "Dirty layer should remain dirty after undo")
            self.assertFalse(rootLayer.dirty, "Root layer dirty state should be restored to pre-flatten state")

            # Verify disk still has the old value 15.0.
            dirtyLayer.Reload()
            self.assertEqual(dirtyLayer.GetPrimAtPath("/Ball3").attributes["radius"].default, 15.0,
                           "Disk should still have the original saved value")


