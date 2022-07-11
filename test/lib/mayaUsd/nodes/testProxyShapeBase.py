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
from pxr import Usd, Sdf

import fixturesUtils

import os
import unittest
import tempfile

import usdUtils, mayaUtils, ufeUtils

import ufe
import mayaUsd.ufe

class testProxyShapeBase(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        cls.mayaSceneFilePath = os.path.join(inputPath, 'ProxyShapeBaseTest',
            'ProxyShapeBase.ma')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testBoundingBox(self):
        cmds.file(self.mayaSceneFilePath, open=True, force=True)

        # Verify that the proxy shape read something from the USD file.
        bboxSize = cmds.getAttr('Cube_usd.boundingBoxSize')[0]
        self.assertEqual(bboxSize, (1.0, 1.0, 1.0))

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testDuplicateProxyStageAnonymous only available in UFE v2 or greater.')
    def testDuplicateProxyStageAnonymous(self):
        '''
        Verify stage with new anonymous layer is duplicated properly.
        '''
        cmds.file(new=True, force=True)

        # create a proxy shape and add a Capsule prim
        import mayaUsd_createStageWithNewLayer
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
        stage = mayaUsd.ufe.getStage(str(proxyShapePath))
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
        duplStage = mayaUsd.ufe.getStage(str(ufe.PathString.path('|stage2|stageShape2')))
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
        # will still leave another and have something to iterate over in th estage cache.
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

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testDuplicateProxyStageFileBacked only available in UFE v2 or greater.')
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
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

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
        duplStage = mayaUsd.ufe.getStage(str(ufe.PathString.path('|Tree_usd1|Tree_usd1Shape')))
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

    def testShareStagePreserveSession(self):
        '''
        Verify share/unshare stage preserves the data in the session layer
        '''
        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # get the stage
        def getStage():
            proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
            self.assertGreater(len(proxyShapes), 0)
            proxyShapePath = proxyShapes[0]
            return mayaUsd.lib.GetPrim(proxyShapePath).GetStage(), proxyShapePath

        # check that the stage is shared and the root is the right one
        stage, proxyShapePath = getStage()
        self.assertTrue(cmds.getAttr('{}.{}'.format(proxyShapePath,"shareStage")))

        # create a prim in the session layer.
        stage.SetEditTarget(stage.GetSessionLayer())
        stage.DefinePrim("/dummy", "xform")

        # verify that the prim exists.
        def verifyPrim():
            stage, _ = getStage()
            self.assertTrue(stage.GetPrimAtPath("/dummy"))

        verifyPrim()

        # unshare the stage and verify the prim in the session layer still exists.
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), False)

        verifyPrim()

        # re-share the stage and verify the prim in the session layer still exists.
        cmds.setAttr('{}.{}'.format(proxyShapePath,"shareStage"), True)

        verifyPrim()

    def testUnsavedStagePreserveRootLayerWhenUpdated(self):
        '''
        Verify that a freshly-created stage preserve its data when an attribute is set.
        '''
        # Create an empty stage.
        cmds.file(new=True, force=True)
        mayaUtils.createProxyAndStage()

        # Helper to get the stage,
        def getStage():
            proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
            self.assertGreater(len(proxyShapes), 0)
            proxyShapePath = proxyShapes[0]
            return mayaUsd.lib.GetPrim(proxyShapePath).GetStage(), proxyShapePath

        # create a prim in the root layer.
        stage, proxyShapePath = getStage()
        stage.SetEditTarget(stage.GetRootLayer())
        stage.DefinePrim("/dummy", "xform")

        # verify that the prim exists.
        def verifyPrim():
            stage, _ = getStage()
            self.assertTrue(stage.GetPrimAtPath("/dummy"))

        verifyPrim()

        # Set an attribute on the proxy shape. Here we set the loadPayloads.
        # It was already set, this only triggers a Maya node recompute.
        cmds.setAttr('{}.{}'.format(proxyShapePath,"loadPayloads"), True)

        # Verify that we did not lose the data on the root layer.
        verifyPrim()

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testShareStageLoadRules only available in UFE v2 or greater.')
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

        # check that the stage has no load rules
        loadRules = stage.GetLoadRules()
        self.assertEqual(len(loadRules.GetRules()), 0)

        # add a new rule to unload the room
        cmd = contextOps.doOpCmd(['Unload'])
        self.assertIsNotNone(cmd)
        cmd.execute()

        # check that the expected load rules are on the stage.
        def check_load_rules(stage):
            loadRules = stage.GetLoadRules()
            self.assertEqual(len(loadRules.GetRules()), 1)
            self.assertEqual(loadRules.GetRules()[0][0], "/Room_set")
            self.assertEqual(loadRules.GetRules()[0][1], loadRules.NoneRule)

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
        testDir = tempfile.mkdtemp(prefix='ProxyShapeBase')
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
        testDir = tempfile.mkdtemp(prefix='ProxyShapeBase')
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


if __name__ == '__main__':
    unittest.main(verbosity=2)
