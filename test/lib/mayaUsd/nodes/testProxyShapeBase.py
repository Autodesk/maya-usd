#!/usr/bin/env mayapy
#
# Copyright 2016 Pixar
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
from pxr import Usd, Sdf, UsdUtils, UsdGeom

import fixturesUtils
import mayaUsd_createStageWithNewLayer

import os
import tempfile
import unittest
import json

import usdUtils, mayaUtils, ufeUtils, testUtils

import ufe
import mayaUsd.ufe

class testProxyShapeBase(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        cls.mayaSceneFilePath = os.path.join(inputPath, 'ProxyShapeBaseTest',
            'ProxyShapeBase.ma')
        cls.usdFilePath = os.path.join(inputPath, 'ProxyShapeBaseTest',
            'CubeModel.usda')
        cls.variantFallbacksUsdFile = os.path.join(inputPath, 'ProxyShapeBaseTest',
            'variantFallbacks.usda')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def getTempFileName(self, filename):
        return os.path.join(
            self._currentTestDir, filename)

    def setupEmptyScene(self):
        self._currentTestDir = tempfile.mkdtemp(prefix='ProxyShapeBaseTest')
        tempMayaFile = self.getTempFileName('ProxyShapeBaseScene.ma')
        cmds.file(new=True, force=True)
        cmds.file(rename=tempMayaFile)
        return tempMayaFile

    def testBoundingBox(self):
        cmds.file(self.mayaSceneFilePath, open=True, force=True)

        # Verify that the proxy shape read something from the USD file.
        bboxSize = cmds.getAttr('Cube_usd.boundingBoxSize')[0]
        self.assertEqual(bboxSize, (1.0, 1.0, 1.0))

    def testDuplicateProxyStageAnonymous(self):
        '''
        Verify stage with new anonymous layer is duplicated properly.
        '''
        cmds.file(new=True, force=True)

        # create a proxy shape and add a Capsule prim
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])

        # proxy shape is expected to have one child
        proxyShapeHier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertEqual(1, len(proxyShapeHier.children()))

        # child name is expected to be Capsule1
        self.assertEqual(str(proxyShapeHier.children()[0].nodeName()), "Capsule1")

        # validate session name and anonymous tag name 
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(proxyShapePath))
        self.assertEqual(stage.GetLayerStack()[0], stage.GetSessionLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())
        self.assertEqual(True, "-session" in stage.GetSessionLayer().identifier)
        self.assertEqual(True, "anonymousLayer1" in stage.GetRootLayer().identifier)

        # Select proxyShapeItem
        proxySelection = ufe.Selection()
        proxySelection.append(proxyShapeItem)
        ufe.GlobalSelection.get().replaceWith(proxySelection)

        # duplicate the proxyShape. 
        cmds.duplicate()

        # get the next item in UFE selection list.
        snIter = iter(ufe.GlobalSelection.get())
        duplStageItem = next(snIter)

        # duplicated stage name is expected to be incremented correctly
        self.assertEqual(str(duplStageItem.nodeName()), "stage2")

        # duplicated proxyShape name is expected to be incremented correctly
        duplProxyShapeHier = ufe.Hierarchy.hierarchy(duplStageItem)
        duplProxyShapeItem = duplProxyShapeHier.children()[0]
        self.assertEqual(str(duplProxyShapeItem.nodeName()), "stageShape2")

        # duplicated ProxyShapeItem should have exactly one child
        self.assertEqual(1, len(ufe.Hierarchy.hierarchy(duplProxyShapeItem).children()))

        # child name is expected to be Capsule1
        childName = str(ufe.Hierarchy.hierarchy(duplProxyShapeItem).children()[0].nodeName())
        self.assertEqual(childName, "Capsule1")

        # validate session name and anonymous tag name 
        duplStage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.PathString.path('|stage2|stageShape2')))
        self.assertEqual(duplStage.GetLayerStack()[0], duplStage.GetSessionLayer())
        self.assertEqual(duplStage.GetEditTarget().GetLayer(), duplStage.GetRootLayer())
        self.assertEqual(True, "-session" in duplStage.GetSessionLayer().identifier)
        self.assertEqual(True, "anonymousLayer1" in duplStage.GetRootLayer().identifier)

        # confirm that edits are not shared (a.k.a divorced). 
        cmds.delete('|stage2|stageShape2,/Capsule1')
        self.assertEqual(0, len(ufe.Hierarchy.hierarchy(duplProxyShapeItem).children()))
        self.assertEqual(1, len(ufe.Hierarchy.hierarchy(proxyShapeItem).children()))

    @unittest.skipUnless(os.getenv('UFE_SCENE_SEGMENT_SUPPORT', '0') > '0', 'testDeleteStage requires sceneSegment support.')
    def testDeleteStage(self):
        '''
        Verify that we can delete the stage.
        
        Only testable in recent UFE API because it requires findGatewayItems
        to trigger the original crash that led to this test being created.
        '''
        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples twice, so that deleting one instance
        # will still leave another and have something to iterate over in the stage cache.
        mayaUtils.openCylinderScene()
        mayaUtils.openCylinderScene()

        # get the proxy shape path
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)

        # Get the USD handler.
        proxyShapePath = ufe.PathString.path(proxyShapes[0])
        handler = ufe.RunTimeMgr.instance().sceneSegmentHandler(proxyShapePath.runTimeId())

        # Delete the proxy shape.
        cmds.delete(proxyShapes[0])

        # Request the now dead proxy shape. This should not crash.
        result = handler.findGatewayItems(proxyShapePath)

    def testDeleteStageUndo(self):
        '''
        Verify that we can undo the deletion of the stage.

        Used to fail when selecting prim afterward, so we validate by using the selection.
        '''
        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples.
        mayaUtils.openCylinderScene()

        # get the proxy shape path
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)

        # select the cylinder. Should not fail.
        cylPath = ufe.Path([
            mayaUtils.createUfePathSegment("|mayaUsdTransform|shape"), 
            usdUtils.createUfePathSegment("/pCylinder1")])
        cylItem = ufe.Hierarchy.createItem(cylPath)
        self.assertIsNotNone(cylItem)
        ufe.GlobalSelection.get().clear()
        ufe.GlobalSelection.get().append(cylItem)

        # Get the USD handler.
        proxyShapePath = ufe.PathString.path(proxyShapes[0])

        # Delete the proxy shape.
        cmds.delete(proxyShapes[0])

        # select the cylinder. Should not fail.
        cylPath = ufe.Path([
            mayaUtils.createUfePathSegment("|mayaUsdTransform|shape"), 
            usdUtils.createUfePathSegment("/pCylinder1")])
        cylItem = ufe.Hierarchy.createItem(cylPath)
        self.assertIsNone(cylItem)

        # Undo the deletion.
        cmds.undo()

        # select the cylinder. Should not fail. (Used to fail.)
        cylItem = ufe.Hierarchy.createItem(cylPath)
        self.assertIsNotNone(cylItem)
        ufe.GlobalSelection.get().clear()
        ufe.GlobalSelection.get().append(cylItem)

    def testDuplicateProxyStageFileBacked(self):
        '''
        Verify stage from file is duplicated properly.
        '''
        # open tree.ma scene
        mayaUtils.openTreeScene()

        # clear selection to start off
        cmds.select(clear=True)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        usdPathSegment = usdUtils.createUfePathSegment('/TreeBase')
        treebasePath = ufe.Path([mayaPathSegment])
        treebaseItem = ufe.Hierarchy.createItem(treebasePath)

        # TreeBase has two children
        self.assertEqual(1, len(ufe.Hierarchy.hierarchy(treebaseItem).children()))

        # get the USD stage
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

        # by default edit target is set to the Rootlayer.
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        # add treebaseItem to selection list
        ufe.GlobalSelection.get().append(treebaseItem)

        # duplicate 
        cmds.duplicate()

        # get the next item in UFE selection list.
        snIter = iter(ufe.GlobalSelection.get())
        duplStageItem = next(snIter)

        # duplicated stage name is expected to be incremented correctly
        self.assertEqual(str(duplStageItem.nodeName()), "Tree_usd1")

        # duplicated proxyShape name is expected to be incremented correctly
        duplProxyShapeHier = ufe.Hierarchy.hierarchy(duplStageItem)
        duplProxyShapeItem = duplProxyShapeHier.children()[0]
        self.assertEqual(str(duplProxyShapeItem.nodeName()), "Tree_usd1Shape")

        # duplicated ProxyShapeItem should have exactly one child
        self.assertEqual(1, len(ufe.Hierarchy.hierarchy(duplProxyShapeItem).children()))

        # child name is expected to be Capsule1
        childName = str(ufe.Hierarchy.hierarchy(duplProxyShapeItem).children()[0].nodeName())
        self.assertEqual(childName, "TreeBase")

        # validate session name and anonymous tag name 
        duplStage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.PathString.path('|Tree_usd1|Tree_usd1Shape')))
        self.assertEqual(duplStage.GetLayerStack()[0], duplStage.GetSessionLayer())
        self.assertEqual(duplStage.GetEditTarget().GetLayer(), duplStage.GetRootLayer())
        self.assertEqual(True, "-session" in duplStage.GetSessionLayer().identifier)
        self.assertEqual(True, "tree.usd" in duplStage.GetRootLayer().identifier)

        # delete "/TreeBase"
        cmds.delete('|Tree_usd1|Tree_usd1Shape,/TreeBase')

        # confirm that edits are shared. Both source and duplicated proxyShapes have no children now.
        self.assertEqual(0, len(ufe.Hierarchy.hierarchy(duplProxyShapeItem).children()))
        self.assertEqual(0, len(ufe.Hierarchy.hierarchy(treebaseItem).children()))

    def testShareStage(self):
        '''
        Verify share/unshare stage workflow works properly
        '''
        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # get the stage
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = proxyShapes[0]
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
        rootIdentifier = stage.GetRootLayer().identifier

        # check that the stage is shared and the root is the right one
        self.assertTrue(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
        self.assertEqual(stage.GetRootLayer().GetDisplayName(), "cylinder.usda")

        # unshare the stage
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), False)
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
        rootLayer = stage.GetRootLayer()

        # check that the stage is now unshared and the root is the anon layer
        # and the old root is now sublayered under that
        self.assertFalse(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
        self.assertEqual(rootLayer.GetDisplayName(), "unshareableLayer")
        self.assertEqual(rootLayer.subLayerPaths, [rootIdentifier])

        # re-share the stage
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), True)
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

        # check that the stage is now shared again and the layer is the same
        self.assertTrue(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
        self.assertEqual(stage.GetRootLayer().GetDisplayName(), "cylinder.usda")

    def testShareStagePreserveFPS(self):
        '''
        Verify share/unshare stage preserve the FPS metadata of the stage.
        '''
        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # get the stage
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = proxyShapes[0]
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
        stage.GetRootLayer().identifier

        # Set an unusual FPS on the stage.
        fps = 45.0
        stage.SetFramesPerSecond(fps)
        stage.GetRootLayer().framesPerSecond = fps

        # Check that the stage FPS is 45.
        self.assertEqual(stage.GetFramesPerSecond(), fps)
        self.assertEqual(stage.GetRootLayer().framesPerSecond, fps)

        # Unshare the stage
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), False)
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
        rootLayer = stage.GetRootLayer()

        # Check that the stage is now unshared and the FPS is still 45.
        self.assertFalse(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
        self.assertEqual(stage.GetFramesPerSecond(), fps)
        self.assertEqual(stage.GetRootLayer().framesPerSecond, fps)

        # Re-share the stage
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), True)
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

        # Check that the stage is now shared again and the FPS is the same.
        self.assertTrue(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
        self.assertEqual(stage.GetFramesPerSecond(), fps)
        self.assertEqual(stage.GetRootLayer().framesPerSecond, fps)

    def testShareStagePreserveTCPS(self):
        '''
        Verify share/unshare stage preserve the TCPS metadata of the stage.
        '''
        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # get the stage
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = proxyShapes[0]
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
        stage.GetRootLayer().identifier

        # Set an unusual TCPS on the stage.
        tcps = 35.0
        stage.SetTimeCodesPerSecond(tcps)
        stage.GetRootLayer().timeCodesPerSecond = tcps

        # Check that the stage TCPS is 35.
        self.assertEqual(stage.GetTimeCodesPerSecond(), tcps)
        self.assertEqual(stage.GetRootLayer().timeCodesPerSecond, tcps)

        # Unshare the stage
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), False)
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
        rootLayer = stage.GetRootLayer()

        # Check that the stage is now unshared and the TCPS is still 35.
        self.assertFalse(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
        self.assertEqual(stage.GetTimeCodesPerSecond(), tcps)
        self.assertEqual(stage.GetRootLayer().timeCodesPerSecond, tcps)

        # Re-share the stage
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), True)
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

        # Check that the stage is now shared again and the TCPS is the same.
        self.assertTrue(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
        self.assertEqual(stage.GetTimeCodesPerSecond(), tcps)
        self.assertEqual(stage.GetRootLayer().timeCodesPerSecond, tcps)

    def _getStage(self):
        '''
        Helper to get the stage, Needed since the stage instance will change
        after saving.
        '''
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        self.assertGreater(len(proxyShapes), 0)
        proxyShapePath = proxyShapes[0]
        return mayaUsd.lib.GetPrim(proxyShapePath).GetStage(), proxyShapePath

    def _defineDummyPrim(self, stage = None, target = None):
        '''
        Define a prim named "dummy". Can be create in an edit target if given one.
        '''
        if stage is None:
            stage, _ = self._getStage()
        if target is not None:
            stage.SetEditTarget(target)
        return stage.DefinePrim("/dummy", "xform")

    def _verifyPrim(self, isActive = True):
        '''
        Verify that the prim named "dummy" exists and its active state.
        '''
        stage, _ = self._getStage()
        prim = stage.GetPrimAtPath("/dummy")
        self.assertTrue(prim)
        self.assertEqual(prim.IsActive(), isActive)

    def _verifySubLayer(self, expectedCount = 3):
        '''
        Verify that the stage still contains a sub-layer under the root.
        Can pass the expected count if the setup is modified, for example
        when the stage is not shared, an extra layer added.
        '''
        stage, _ = self._getStage()
        stack = stage.GetLayerStack()
        # Layer stack: session, root, sub-layer
        self.assertEqual(expectedCount, len(stack))

    def testShareStagePreserveSession(self):
        '''
        Verify share/unshare stage preserves the data in the session layer
        '''
        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # check that the stage is shared and the root is the right one
        stage, proxyShapePath = self._getStage()
        self.assertTrue(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))

        # create a prim in the session layer.
        self._defineDummyPrim(stage, stage.GetSessionLayer())
        self._verifyPrim()

        # unshare the stage and verify the prim in the session layer still exists.
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), False)

        self._verifyPrim()

        # re-share the stage and verify the prim in the session layer still exists.
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), True)

        self._verifyPrim()

    def testForTArgetSessionLayer(self):
        '''
        Verify that setting the option var mayaUsd_ProxyTargetsSessionLayerOnOpen
        to 1 targets the session layer instead of the layer that was previously targeted.
        '''
        cmds.optionVar(iv=["mayaUsd_ProxyTargetsSessionLayerOnOpen",  1])

        try:
            # create new stage
            cmds.file(new=True, force=True)

            # Open target-root-layerrma scene in testSamples
            if Usd.GetVersion() < (0, 25, 8):
                mayaUtils.openTestScene("targetRootLayer", "target-root-layer.ma")
            else:
                mayaUtils.openTestScene("targetRootLayer", "target-root-layer-usda-2508.ma")

            # check that the stage is shared and the root is the right one
            stage, proxyShapePath = self._getStage()

            self.assertTrue(stage)
            self.assertEqual(stage.GetSessionLayer().identifier, stage.GetEditTarget().GetLayer().identifier)
        finally:
            cmds.optionVar(iv=["mayaUsd_ProxyTargetsSessionLayerOnOpen",  0])

    def _saveStagePreserveLayerHelper(self, targetRoot, saveInMaya):
        '''
        Verify that a freshly-created stage preserve its session or root layer data
        when saved to Maya or USD files.

        (What happens internally is that the root layer changes name when saved,
        so the stage cache needs to be updated by the save code in order to end-up
        with the same session layer.)
        '''
        # Create an empty stage.
        cmds.file(new=True, force=True)
        mayaUtils.createProxyAndStage()

        stage, proxyShapePath = self._getStage()

        # Create an anonymous sub-layer.
        subLayer = Sdf.Layer.CreateAnonymous("middleLayer")
        stage.GetRootLayer().subLayerPaths  = [subLayer.identifier]

        # Create a prim in the root layer.
        stage.SetEditTarget(stage.GetRootLayer())
        stage.DefinePrim("/dummy", "xform")

        # Make the prim inactive in the desired target layer.
        target = stage.GetRootLayer() if targetRoot else stage.GetSessionLayer()
        prim = self._defineDummyPrim(stage, target)
        prim.SetActive(False)

        # verify that the prim exists but is inactive.
        self._verifyPrim(False)

        # Temp file names for Maya scene and USD file.
        with testUtils.TemporaryDirectory(prefix='ProxyShapeBase', ignore_errors=True) as testDir:
            targetName = 'Root' if targetRoot else 'Session'
            inMayaName = 'InMaya' if saveInMaya else 'InUSD'
            tempMayaFile = os.path.join(testDir, 'SaveStagePreserve%s%s.ma' % (targetName, inMayaName))

            # Make sure layers are saved to the desired location (Maya or USD)
            # when the Maya scene is saved.
            location = 2 if saveInMaya else 1
            cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', location))

            # Save the stage.
            cmds.file(rename=tempMayaFile)
            cmds.file(save=True, force=True, type='mayaAscii')

            # Verify that the prim is still inactive in the target layer.
            self._verifyPrim(False)
            self._verifySubLayer()

            # Reload the file and verify again.
            cmds.file(new=True, force=True)
            cmds.file(tempMayaFile, force=True, open=True)

            self._verifyPrim(False)
            self._verifySubLayer()

            # Change shared status and verify again. Changing the shared flag recomputes
            # the stage and its layer, which is what we really want to test here. So we
            # change an input attribute that we know will recompute the layers.
            cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), False)
    
            # Verify that the prim is still inactive in the target layer.
            self._verifyPrim(False)
            self._verifySubLayer(4)

            cmds.file(new=True, force=True)

    def testSaveStageToUSDPreserveSessionLayer(self):
        '''
        Verify that a freshly-created stage preserve its session layer data when saved
        to USD files.
        '''
        self._saveStagePreserveLayerHelper(False, False)

    def testSaveStageToMayaPreserveSessionLayer(self):
        '''
        Verify that a freshly-created stage preserve its session layer data when saved
        to Maya.
        '''
        self._saveStagePreserveLayerHelper(False, True)

    def testSaveStageToUSDPreserveRootLayer(self):
        '''
        Verify that a freshly-created stage preserve its root layer data when saved
        to USD files.
        '''
        self._saveStagePreserveLayerHelper(True, False)

    def testSaveStageToMayaPreserveRootLayer(self):
        '''
        Verify that a freshly-created stage preserve its root layer data when saved
        to Maya.
        '''
        self._saveStagePreserveLayerHelper(True, True)

    def testUnsavedStagePreserveRootLayerWhenUpdated(self):
        '''
        Verify that a freshly-created stage preserve its data when an attribute is set.
        '''
        # Create an empty stage.
        cmds.file(new=True, force=True)
        mayaUtils.createProxyAndStage()

        # create a prim in the root layer.
        self._defineDummyPrim()

        # verify that the prim exists.
        self._verifyPrim()

        # Set an attribute on the proxy shape. Here we set the shareStage.
        # It was already set, this only triggers a Maya node recompute.
        _, proxyShapePath = self._getStage()
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), True)

        # Verify that we did not lose the data on the root layer.
        self._verifyPrim()

    def testSettingStageViaIdPreservedWhenSaved(self):
        '''
        Verify that setting the stage via the stageCacheId on the proxy shape can be reloaded.
        '''
        cmds.file(new=True, force=True)

        # Create a USD stage directly using USD.
        filePath = testUtils.getTestScene('cylinder', 'cylinder.usda')
        stageCache = UsdUtils.StageCache.Get()
        with Usd.StageCacheContext(stageCache):
            cachedStage = Usd.Stage.Open(filePath)

        # Create a proxy shape and sets its stage ID attribute.           
        stageId = stageCache.GetId(cachedStage).ToLongInt()
        cmds.createNode('mayaUsdProxyShape')
        shapeNode = cmds.ls(sl=True,l=True)[0]
        cmds.setAttr('{}.stageCacheId'.format(shapeNode), stageId)

        # Verify we can access the stage.
        def verify():
            item = ufeUtils.createUfeSceneItem(shapeNode,'/pCylinder1')
            self.assertIsNotNone(item)

        verify()

        # Save the maya file
        with testUtils.TemporaryDirectory(prefix='ProxyShapeBase', ignore_errors=True) as testDir:
            # Make sure layers are saved to the desired location
            # when the Maya scene is saved.
            saveInUSD = 1
            cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', saveInUSD))

            # Save the stage.
            tempMayaFile = os.path.join(testDir, 'StageViaIdPreservedWhenSaved')
            cmds.file(rename=tempMayaFile)
            cmds.file(save=True, force=True, type='mayaAscii')
        
            cmds.file(new=True, force=True)
            cmds.file(tempMayaFile, force=True, open=True)

            verify()


    def testShareStageLoadRules(self):
        '''
        Verify that share/unshare stage preserve the load rules of the stage.
        '''
        # create new stage
        cmds.file(new=True, force=True)

        # open a scene with payload
        mayaUtils.openTopLayerScene()
        
        # select the room and get its context menu.
        roomPath = ufe.Path([
            mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set")])
        roomItem = ufe.Hierarchy.createItem(roomPath)
        ufe.GlobalSelection.get().clear()
        ufe.GlobalSelection.get().append(roomItem)
        contextOps = ufe.ContextOps.contextOps(roomItem)

        # get the stage
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = proxyShapes[0]
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

        # check that the stage has a single load rule to laod everything.
        loadRules = stage.GetLoadRules()
        self.assertEqual(len(loadRules.GetRules()), 1)
        self.assertEqual(loadRules.GetRules()[0][0], "/")
        self.assertEqual(loadRules.GetRules()[0][1], loadRules.AllRule)

        # add a new rule to unload the room
        cmd = contextOps.doOpCmd(['Unload'])
        self.assertIsNotNone(cmd)
        cmd.execute()

        # check that the expected load rules are on the stage.
        def check_load_rules(stage):
            loadRules = stage.GetLoadRules()
            self.assertEqual(len(loadRules.GetRules()), 2)
            self.assertEqual(loadRules.GetRules()[0][0], "/")
            self.assertEqual(loadRules.GetRules()[0][1], loadRules.AllRule)
            self.assertEqual(loadRules.GetRules()[1][0], "/Room_set")
            self.assertEqual(loadRules.GetRules()[1][1], loadRules.NoneRule)

        check_load_rules(stage)

        # unshare the stage
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), False)
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

        # check that the expected load rules are still on the stage.
        check_load_rules(stage)

        # re-share the stage
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), True)
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

        # check that the expected load rules are still on the stage.
        check_load_rules(stage)

    def testStageMutedLayers(self):
        '''
        Verify that stage preserve the muted layers of the stage when a scene is reloaded.
        '''
        # create new scene
        tempMayaFile = self.setupEmptyScene()

        # create an empty scene
        shapePath = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(shapePath).GetStage()

        # Verify there are no muted layers by default.
        self.assertListEqual([], stage.GetMutedLayers())

        # Change the muted layers.
        stage.MuteLayer("abc")
        stage.MuteLayer("def")

        def verifyMuting(stage):
            self.assertListEqual(["abc", "def"], stage.GetMutedLayers())

        verifyMuting(stage)

        # Save the Maya scene.
        cmds.file(save=True, force=True)

        # Reload the scene and verify the muting are still there.
        cmds.file(new=True, force=True)
        cmds.file(tempMayaFile, open=True)

        stage = mayaUsd.lib.GetPrim('|stage1|stageShape1').GetStage()
        verifyMuting(stage)

    def testStageTargetLayer(self):
        '''
        Verify that stage preserve the target layer of the stage when a scene is reloaded.
        '''
        # create new scene
        tempMayaFile = self.setupEmptyScene()

        # create an empty scene
        shapePath = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(shapePath).GetStage()

        # Add a sub-layer and target it.
        subLayerFileName = self.getTempFileName('targetSubLayer.usd')
        usdFormat = Sdf.FileFormat.FindByExtension('usd')
        subLayer = Sdf.Layer.New(usdFormat, subLayerFileName)
        subLayer.Save()
        stage.GetRootLayer().subLayerPaths.append(subLayer.identifier)
        stage.SetEditTarget(subLayer)

        def verifyTargetLayer(stage):
            self.assertNotEqual(stage.GetRootLayer().identifier, stage.GetEditTarget().GetLayer().identifier)

        verifyTargetLayer(stage)

        # Save the Maya scene.
        cmds.file(save=True, force=True)

        # Reload the scene and verify the target layer is not the root layer.
        cmds.file(new=True, force=True)
        cmds.file(tempMayaFile, open=True)

        stage = mayaUsd.lib.GetPrim('|stage1|stageShape1').GetStage()
        self.assertListEqual(list(stage.GetRootLayer().subLayerPaths), [subLayer.identifier])
        verifyTargetLayer(stage)

    def testStageAnonymousSubLayerAsTargetLayer(self):
        '''
        Verify that stage preserve the anonymous sub layer edit target layer when a scene is reloaded.
        '''
        # Create new scene
        cmds.file(new=True, force=True)

        # Create an empty scene
        shapePath = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(shapePath).GetStage()

        stage.SetEditTarget(stage.GetSessionLayer())

        # Add a sub-layer and target it.
        subLayer = Sdf.Layer.CreateAnonymous()
        stage.GetSessionLayer().subLayerPaths.append(subLayer.identifier)
        stage.SetEditTarget(subLayer)

        def verifyTargetLayer(stage):
            self.assertNotEqual(stage.GetSessionLayer().identifier, stage.GetEditTarget().GetLayer().identifier)
            self.assertNotEqual(stage.GetRootLayer().identifier, stage.GetEditTarget().GetLayer().identifier)

        verifyTargetLayer(stage)

        # Save and re-open
        with testUtils.TemporaryDirectory(prefix='ProxyShapeBase') as testDir:
            # Save the dirty layer along with Maya scene
            cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', 2))
            tempMayaFile = os.path.join(testDir, 'StageAnonymousSubLayerAsTargetLayer.ma')
            cmds.file(rename=tempMayaFile)
            cmds.file(save=True, force=True)

            # Save the Maya scene.
            cmds.file(new=True, force=True)
            cmds.file(tempMayaFile, open=True)

            stage = mayaUsd.lib.GetPrim('|stage1|stageShape1').GetStage()
            self.assertEqual(len(list(stage.GetSessionLayer().subLayerPaths)), 1)
            verifyTargetLayer(stage)

            subLayer = stage.GetSessionLayer().subLayerPaths[0]

            self.assertTrue(Sdf.Layer.IsAnonymousLayerIdentifier(subLayer))
            self.assertEqual(stage.GetEditTarget().GetLayer().identifier, subLayer)

            cmds.file(new=True, force=True)

    def testStageAnonymousRootLayerInMaya(self):
        '''
        Verify that stage preserve the anonymous root layer of the stage when a scene is reloaded.
        '''
        # Create new scene
        cmds.file(new=True, force=True)

        # Prepare anonymous root layer
        anonRootLayer = Sdf.Layer.CreateAnonymous()
        anonRootLayerId = anonRootLayer.identifier
        primSpec = Sdf.CreatePrimInLayer(anonRootLayer, '/root_xform')
        primSpec.specifier = Sdf.SpecifierDef
        primSpec.typeName = 'Xform'

        # Create an empty proxy shape
        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        # Set root layer file path to the newly created anonymous layer
        cmds.setAttr('|stage1|stageShape1.filePath', anonRootLayer.identifier, type='string')
        filePath = cmds.getAttr('|stage1|stageShape1.filePath')
        self.assertEqual(anonRootLayerId, filePath)

        stage = mayaUsd.lib.GetPrim('|stage1|stageShape1').GetStage()
        stage.SetEditTarget(stage.GetRootLayer())
        self.assertTrue(stage.GetPrimAtPath('/root_xform'))

        # Save and re-open
        with testUtils.TemporaryDirectory(prefix='ProxyShapeBase') as testDir:
            # Save the dirty layer along with Maya scene
            cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', 2))
            tempMayaFile = os.path.join(testDir, 'AnonymousRootLayerTest.ma')
            cmds.file(rename=tempMayaFile)
            cmds.file(save=True, force=True)

            # Save the Maya scene.
            cmds.file(new=True, force=True)
            cmds.file(tempMayaFile, open=True)

            stage = mayaUsd.lib.GetPrim('|stage1|stageShape1').GetStage()
            self.assertEqual(stage.GetRootLayer().identifier, stage.GetEditTarget().GetLayer().identifier)

            # Verify the root layer has been recreated and content restored
            self.assertTrue(stage.GetPrimAtPath('/root_xform'))

            # Verify the .filePath attribute string
            filePath = cmds.getAttr('|stage1|stageShape1.filePath')
            self.assertTrue(Sdf.Layer.IsAnonymousLayerIdentifier(filePath))
            self.assertEqual(stage.GetRootLayer().identifier, filePath)
            self.assertNotEqual(anonRootLayerId, filePath)

            cmds.file(new=True, force=True)

    def testSerializationShareStage(self):
        '''
        Verify share/unshare stage works with serialization and complex heirharchies
        '''
        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # get the stage
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = proxyShapes[0]
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
        originalRootIdentifier = stage.GetRootLayer().identifier

        # check that the stage is shared and the root is the right one
        self.assertTrue(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
        self.assertEqual(stage.GetRootLayer().GetDisplayName(), "cylinder.usda")

        # unshare the stage
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), False)
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
        rootLayer = stage.GetRootLayer()

        # check that the stage is now unshared and the root is the anon layer
        # and the old root is now sublayered under that
        self.assertFalse(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
        self.assertEqual(rootLayer.GetDisplayName(), "unshareableLayer")
        self.assertEqual(rootLayer.subLayerPaths, [originalRootIdentifier])

        middleLayer = Sdf.Layer.CreateAnonymous("middleLayer")
        middleLayer.subLayerPaths = [originalRootIdentifier]
        rootLayer.subLayerPaths  = [middleLayer.identifier]

        # Save and re-open
        with testUtils.TemporaryDirectory(prefix='ProxyShapeBase') as testDir:
            tempMayaFile = os.path.join(testDir, 'ShareStageSerializationTest.ma')
            cmds.file(rename=tempMayaFile)
            # make the USD layer absolute otherwise it won't be found
            cmds.setAttr('{}.{}'.format(proxyShapePath,"filePath"), originalRootIdentifier, type='string')
            cmds.file(save=True, force=True)
            cmds.file(new=True, force=True)
            cmds.file(tempMayaFile, open=True)

            # get the stage again (since we opened a new file)
            proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
            proxyShapePath = proxyShapes[0]
            stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
            rootLayer = stage.GetRootLayer()

            # make sure the middle layer is back (only one)
            self.assertEqual(len(rootLayer.subLayerPaths), 1)
            middleLayer = Sdf.Layer.Find(rootLayer.subLayerPaths[0])
            self.assertEqual(middleLayer.GetDisplayName(), "middleLayer")

            # make sure the middle layer still contains the original root only
            self.assertEqual(len(middleLayer.subLayerPaths), 1)
            self.assertEqual(middleLayer.subLayerPaths[0], originalRootIdentifier)

            # re-share the stage
            cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), True)
            stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

            # check that the stage is now shared again and the identifier is correct
            self.assertTrue(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
            self.assertEqual(stage.GetRootLayer().identifier, originalRootIdentifier)

            cmds.file(new=True, force=True)

    def testShareStageComplexHierarchyToggle(self):
        '''
        Verify share/unshare stage toggle works with complex heirharchies
        '''
        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # get the stage
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = proxyShapes[0]
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
        originalRootIdentifier = stage.GetRootLayer().identifier

        # check that the stage is shared and the root is the right one
        self.assertTrue(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
        self.assertEqual(stage.GetRootLayer().GetDisplayName(), "cylinder.usda")

        # unshare the stage
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), False)
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
        rootLayer = stage.GetRootLayer()

        # check that the stage is now unshared and the root is the anon layer
        # and the old root is now sublayered under that
        self.assertFalse(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
        self.assertEqual(rootLayer.GetDisplayName(), "unshareableLayer")
        self.assertEqual(rootLayer.subLayerPaths, [originalRootIdentifier])

        middleLayer = Sdf.Layer.CreateAnonymous("middleLayer")
        middleLayer.subLayerPaths = [originalRootIdentifier]
        rootLayer.subLayerPaths  = [middleLayer.identifier]

         # Save and re-open
        with testUtils.TemporaryDirectory(prefix='ProxyShapeBase') as testDir:
            tempMayaFile = os.path.join(testDir, 'ShareStageComplexHierarchyToggle.ma')
            cmds.file(rename=tempMayaFile)
            # make the USD layer absolute otherwise it won't be found
            cmds.setAttr('{}.{}'.format(proxyShapePath,"filePath"), originalRootIdentifier, type='string')
            cmds.file(save=True, force=True)
            cmds.file(new=True, force=True)
            cmds.file(tempMayaFile, open=True)

            # get the stage again (since we opened a new file)
            proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
            proxyShapePath = proxyShapes[0]
            stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
            rootLayer = stage.GetRootLayer()

            # make sure the middle layer is back (only one)
            self.assertEqual(len(rootLayer.subLayerPaths), 1)
            middleLayer = Sdf.Layer.Find(rootLayer.subLayerPaths[0])
            self.assertEqual(middleLayer.GetDisplayName(), "middleLayer")

            # re-share the stage
            cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), True)
            stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

            # check that the stage is now shared again and the identifier is correct
            self.assertTrue(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
            self.assertEqual(stage.GetRootLayer().identifier, originalRootIdentifier)

            # unshare the stage
            cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), False)
            stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
            rootLayer = stage.GetRootLayer()

            # check that the stage is now shared and the root is the anon layer
            # and the old root is now sublayered under that
            self.assertFalse(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))
            self.assertEqual(rootLayer.GetDisplayName(), "unshareableLayer")
            self.assertEqual(rootLayer.subLayerPaths, [middleLayer.identifier])

            cmds.file(new=True, force=True)

    def testShareStageSourceChange(self):
        '''
        Verify the stage source change maintains the position in the hierarchy
        '''
        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # get the proxy shape path
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapeA = proxyShapes[0]

        # create another proxyshape (B)
        import mayaUsd_createStageWithNewLayer
        proxyShapeB = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapeB = proxyShapeB.split("|")[-1]

        # Connect them using stage data
        cmds.connectAttr('{}.outStageData'.format(proxyShapeA),
                         '{}.inStageData'.format(proxyShapeB))

        # get the stage
        stageB = mayaUsd.lib.GetPrim(proxyShapeB).GetStage()
        originalRootIdentifierB = stageB.GetRootLayer().identifier

        # check that the stage is shared and the root is the right one
        self.assertTrue(cmds.getAttr('{}.{}'.format(proxyShapeB,"shareStage")))
        self.assertEqual(stageB.GetRootLayer().GetDisplayName(), "cylinder.usda")

        # unshare the stage
        cmds.setAttr('{}.{}'.format(proxyShapeB,"shareStage"), False)
        stageB = mayaUsd.lib.GetPrim(proxyShapeB).GetStage()
        rootLayerB = stageB.GetRootLayer()

        # check that the stage is now unshared and the root is the anon layer
        # and the old root is now sublayered under that
        self.assertFalse(cmds.getAttr('{}.{}'.format(proxyShapeB,"shareStage")))
        self.assertEqual(rootLayerB.GetDisplayName(), "unshareableLayer")
        self.assertEqual(rootLayerB.subLayerPaths, [originalRootIdentifierB])

        # Add complex hierarchy
        middleLayerB = Sdf.Layer.CreateAnonymous("middleLayer")
        middleLayerB.subLayerPaths = [originalRootIdentifierB]
        rootLayerB.subLayerPaths  = [middleLayerB.identifier]

        # unshare the stage from the first proxy
        cmds.setAttr('{}.{}'.format(proxyShapeA, "shareStage"), False)
        stageA = mayaUsd.lib.GetPrim(proxyShapeA).GetStage()
        rootLayerA = stageA.GetRootLayer()
        stageB = mayaUsd.lib.GetPrim(proxyShapeB).GetStage()
        rootLayerB = stageB.GetRootLayer()

        # check that the stage is now unshared and the entire hierachy is good (A)
        self.assertFalse(cmds.getAttr('{}.{}'.format(proxyShapeA,"shareStage")))
        self.assertEqual(rootLayerA.GetDisplayName(), "unshareableLayer")
        self.assertEqual(len(rootLayerA.subLayerPaths), 1)
        self.assertEqual(rootLayerA.subLayerPaths[0], originalRootIdentifierB)

        # Make sure the hierachy is good (B)
        self.assertFalse(cmds.getAttr('{}.{}'.format(proxyShapeB,"shareStage")))
        self.assertEqual(rootLayerB.GetDisplayName(), "unshareableLayer")
        self.assertEqual(len(rootLayerB.subLayerPaths), 1)
        middleLayer = Sdf.Layer.Find(rootLayerB.subLayerPaths[0])
        self.assertEqual(middleLayer.GetDisplayName(), "middleLayer")
        self.assertEqual(len(middleLayer.subLayerPaths), 1)
        unshareableLayerFromA = Sdf.Layer.Find(middleLayer.subLayerPaths[0])
        self.assertEqual(unshareableLayerFromA.GetDisplayName(), "unshareableLayer")
        self.assertEqual(len(unshareableLayerFromA.subLayerPaths), 1)
        self.assertEqual(unshareableLayerFromA.subLayerPaths[0], originalRootIdentifierB)

    def testCachedStage(self):
        '''
        Test saving cached stage
        '''
        stageCache = UsdUtils.StageCache.Get()
        with Usd.StageCacheContext(stageCache):
            cachedStage = Usd.Stage.Open(self.usdFilePath)

        stageId = stageCache.GetId(cachedStage).ToLongInt()
        shapeNode = cmds.createNode('mayaUsdProxyShape')
        cmds.setAttr('{}.stageCacheId'.format(shapeNode), stageId)
        cachedStage.SetEditTarget(cachedStage.GetSessionLayer())

        fullPath = cmds.ls(shapeNode, long=True)[0]
        cmds.select("{},/CubeModel".format(fullPath))
        # Move the cube prim
        cmds.move(0, 0, 2, r=True)

        # Make sure shareStage is ON
        self.assertTrue(cmds.getAttr('{}.{}'.format(shapeNode,"shareStage")))

        # Make sure there is no connection to inStageData
        self.assertIsNone(cmds.listConnections('{}.inStageData'.format(shapeNode), s=True, d=False))

        cmds.select(cmds.listRelatives(fullPath, p=True)[0], r=True)

        with testUtils.TemporaryDirectory(prefix='ProxyShapeBase') as testDir:
            pathToSave = "{}/CubeModel.ma".format(testDir)

            # Make sure to save USD back to Maya file, so that the USD test
            # file we loaded (from source folder) isn't modified.
            cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', 2))

            cmds.file(rename=pathToSave)
            cmds.file(save=True, force=True, type='mayaAscii')
            
            cmds.file(new=True, force=True)
            cmds.file(pathToSave, force=True, open=True)

            stage = mayaUsd.lib.GetPrim(fullPath).GetStage()
            prim = stage.GetPrimAtPath('/CubeModel')
            self.assertTrue(prim.IsValid())

            # Verify we get the saved changes we did 
            xform = UsdGeom.Xformable(prim)
            translate = xform.GetOrderedXformOps()[0].Get()
            self.assertEqual(translate[2], 2)

    def testRegisterFilePathEditor(self):
        '''
        Test registering USD file to Maya file path editor
        '''
        # create new stage
        cmds.file(new=True, force=True)
        with testUtils.TemporaryDirectory(prefix='ProxyShapeBase') as testDir:
            pathToSave = os.path.join(testDir, 'testRegisterFilePathEditor.usda')
            cmds.file(rename=pathToSave)            
            cmds.file(save=True, force=True)
            cmds.file(new=True, force=True)

        layer = Sdf.Layer.CreateNew(pathToSave)
        Sdf.CreatePrimInLayer(layer, "/root")
        layer.Save()
        node = cmds.createNode("mayaUsdProxyShape")
        cmds.setAttr("{node}.filePath".format(node=node), layer.realPath, type="string")
        directories = cmds.filePathEditor(query=True, listDirectories="", unresolved=True)
        self.assertIsNotNone(directories)

    def testSavingVariantFallbacks(self):
        '''
        Test saving custom Global Variant Fallbacks to mayaUsdProxyShape.
        '''
        class VariantFallbackOverrides(object):
            def __init__(self, overrides):
                self._overrides = overrides or {}
                self._defaultVariantFallbacks = None

            def __enter__(self):
                self._defaultVariantFallbacks = Usd.Stage.GetGlobalVariantFallbacks()
                fallbacks = self._defaultVariantFallbacks.copy()
                fallbacks.update(self._overrides)
                Usd.Stage.SetGlobalVariantFallbacks(fallbacks)

            def __exit__(self, type, value, traceback):
                if self._defaultVariantFallbacks:
                    Usd.Stage.SetGlobalVariantFallbacks(self._defaultVariantFallbacks)

        def _verifyVariantFallbacks(primPath, stage, shapeNode, overrides):
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim.IsValid())

            variantFallbackData = json.loads(cmds.getAttr('{}.variantFallbacks'.format(shapeNode)))
            for k, value in overrides.items():
                self.assertEqual(variantFallbackData.get(k, None), value)

            variantSets = prim.GetVariantSets()
            for name in variantSets.GetNames():
                self.assertEqual(variantSets.GetVariantSet(name).GetVariantSelection(), overrides[name][0])

        overrides = {'geo': ['render_high'], 'geo_vis': ['preview']}

        SC = UsdUtils.StageCache.Get()
        SC.Clear()

        with VariantFallbackOverrides(overrides):
            with Usd.StageCacheContext(SC):
                cachedStage = Usd.Stage.Open(self.variantFallbacksUsdFile)

            cachedStage.SetEditTarget(cachedStage.GetSessionLayer())
            customLayerData = cachedStage.GetSessionLayer().customLayerData
            customLayerData["variant_fallbacks"] = json.dumps(
                Usd.Stage.GetGlobalVariantFallbacks()
            )
            cachedStage.GetSessionLayer().customLayerData = customLayerData

        stageId = SC.GetId(cachedStage).ToLongInt()
        shapeNode = cmds.createNode('mayaUsdProxyShape')
        fullPath = cmds.ls(shapeNode, long=True)[0]
        cmds.connectAttr('time1.outTime','{}.time'.format(shapeNode))
        cmds.setAttr('{}.stageCacheId'.format(shapeNode), stageId)

        layerdata = cachedStage.GetSessionLayer().customLayerData
        variantFallbacks = layerdata.get("variant_fallbacks")
        cmds.setAttr('{}.variantFallbacks'.format(shapeNode), str(variantFallbacks), type='string')

        _verifyVariantFallbacks('/group', cachedStage, shapeNode, overrides)

        with testUtils.TemporaryDirectory(prefix='ProxyShapeBase') as testDir:
            pathToSave = "{}/testSavingVariantFallbacks.ma".format(testDir)

            # Make sure to save USD back to Maya file, so that the USD test
            # file we loaded (from source folder) isn't modified.
            cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', 2))

            cmds.file(rename=pathToSave)
            cmds.file(save=True, force=True, type='mayaAscii')

            cmds.file(new=True, force=True)
            cmds.file(pathToSave, force=True, open=True)

            stage = mayaUsd.ufe.getStage(fullPath)
            _verifyVariantFallbacks('/group', stage, shapeNode, overrides)

if __name__ == '__main__':
    unittest.main(verbosity=2)
