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

import fixturesUtils
import mayaUtils
import usdUtils
import ufeUtils
import testUtils
import mayaUsd
import mayaUsd_createStageWithNewLayer

from pxr import UsdGeom
from pxr import UsdShade
from pxr import Sdf
from pxr import Usd
from pxr import Vt

from maya import cmds
from maya import standalone
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

import maya.api.OpenMaya as om

import ufe
import usdUfe

import os
import unittest

try:
    from pxr import UsdMtlx
    USD_HAS_MATERIALX_PLUGIN=True
except ImportError:
    USD_HAS_MATERIALX_PLUGIN=False


class TestAddPrimObserver(ufe.Observer):
    def __init__(self):
        super(TestAddPrimObserver, self).__init__()
        self.deleteNotif = 0
        self.addNotif = 0

    def __call__(self, notification):
        if isinstance(notification, ufe.ObjectDelete):
            self.deleteNotif += 1
        if isinstance(notification, ufe.ObjectAdd):
            self.addNotif += 1

    def nbDeleteNotif(self):
        return self.deleteNotif

    def nbAddNotif(self):
        return self.addNotif

    def reset(self):
        self.addNotif = 0
        self.deleteNotif = 0

class ContextOpsTestCase(unittest.TestCase):
    '''Verify the ContextOps interface for the USD runtime.'''

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
        ''' Called initially to set up the maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # These tests requires no additional setup.
        if self._testMethodName in ['testAddNewPrim', 'testAddNewPrimWithDelete']:
            return

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Clear selection to start off.
        ufe.GlobalSelection.get().clear()
        
        # Select Ball_35.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        self.ball35Item = ufe.Hierarchy.createItem(ball35Path)
        self.ball35Prim = usdUtils.getPrimFromSceneItem(self.ball35Item)

        ufe.GlobalSelection.get().append(self.ball35Item)

        # Create a ContextOps interface for it.
        self.contextOps = ufe.ContextOps.contextOps(self.ball35Item)

    def testGetItems(self):
        # The top-level context items are:
        # - working set management for loadable prims and descendants
        # - visibility (for all prims with visibility attribute)
        # - variant sets, for those prims that have them (such as Ball_35)
        # - add new prim (for all prims)
        # - prim activate/deactivate
        # - prim mark/unmark as instanceable
        contextItems = self.contextOps.getItems([])
        contextItemStrings = [c.item for c in contextItems]

        attrs = ufe.Attributes.attributes(self.contextOps.sceneItem())
        self.assertIsNotNone(attrs)
        hasVisibility = attrs.hasAttribute(UsdGeom.Tokens.visibility)

        # The proxy shape specifies loading all payloads, so Ball_35 should be
        # loaded, and since it has a payload, there should be an item for
        # unloading it.
        self.assertIn('Unload', contextItemStrings)
        self.assertIn('Variant Sets', contextItemStrings)
        if hasVisibility:
            self.assertIn('Toggle Visibility', contextItemStrings)
        else:
            self.assertNotIn('Toggle Visibility', contextItemStrings)
        self.assertIn('Toggle Active State', contextItemStrings)
        self.assertIn('Toggle Instanceable State', contextItemStrings)
        self.assertIn('Add New Prim', contextItemStrings)

        # Ball_35 has a single variant set, which has children.
        contextItems = self.contextOps.getItems(['Variant Sets'])
        contextItemStrings = [c.item for c in contextItems]

        self.assertListEqual(['shadingVariant'], contextItemStrings)
        self.assertTrue(contextItems[0].hasChildren)

        # Initial shadingVariant is "Ball_8"
        contextItems = self.contextOps.getItems(
            ['Variant Sets', 'shadingVariant'])
        self.assertGreater(len(contextItems), 1)
        for c in contextItems:
            self.assertFalse(c.hasChildren)
            self.assertTrue(c.checkable)
            if c.checked:
                self.assertEqual(c.item, 'Ball_8')

    def testGetBulkItems(self):
        # The top-level context items are:
        #   Bulk Menu Header
        #   -----------------
        #   Make Visible
        #   Make Invisible
        #   Activate Prim
        #   Deactivate Prim
        #   Mark as Instanceable
        #   Unmark as Instanceable
        #   -----------------
        #   Assign New Material
        #   Assign Existing Material
        #   Unassign Material

        # The default setup selects a single prim (Ball_35) and creates context item
        # for it. We want multiple selection to make a bulk context.
        for ball in ['Ball_32', 'Ball_33', 'Ball_34']:
            ballPath = ufe.Path([
                mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
                usdUtils.createUfePathSegment("/Room_set/Props/%s" % ball)])
            ballItem = ufe.Hierarchy.createItem(ballPath)
            ufe.GlobalSelection.get().append(ballItem)

        # Re-create a ContextOps interface for it.
        # Since the context item is in the selection, we get a bulk context.
        self.contextOps = ufe.ContextOps.contextOps(self.ball35Item)

        contextItems = self.contextOps.getItems([])
        contextItemStrings = [c.item for c in contextItems]

        # Special header at top of menu.
        self.assertIn('BulkEdit', contextItemStrings)

        # Not supported in bulk (from UsdContextOps).
        self.assertNotIn('Load', contextItemStrings)
        self.assertIn('Load with Descendants', contextItemStrings)
        self.assertIn('Unload', contextItemStrings)
        self.assertNotIn('Variant Sets', contextItemStrings)
        self.assertNotIn('Add New Prim', contextItemStrings)

        # Two actions in bulk, instead of toggle.
        self.assertIn('Make Visible', contextItemStrings)
        self.assertIn('Make Invisible', contextItemStrings)
        self.assertNotIn('Toggle Visibility', contextItemStrings)

        self.assertIn('Activate Prim', contextItemStrings)
        self.assertIn('Deactivate Prim', contextItemStrings)
        self.assertNotIn('Toggle Active State', contextItemStrings)

        self.assertIn('Mark as Instanceable', contextItemStrings)
        self.assertIn('Unmark as Instanceable', contextItemStrings)
        self.assertNotIn('Toggle Instanceable State', contextItemStrings)

        if ufeUtils.ufeFeatureSetVersion() >= 4:
            self.assertIn('Assign New Material', contextItemStrings)
            # Because there are no materials in the scene we don't have this item.
            self.assertNotIn('Assign Existing Material', contextItemStrings)
            self.assertIn('Unassign Material', contextItemStrings)

        # Not supported in bulk (from MayaUsdContextOps).
        self.assertNotIn('Assign Material to Selection', contextItemStrings)
        self.assertNotIn('USD Layer Editor', contextItemStrings)
        self.assertNotIn('Edit As Maya Data', contextItemStrings)
        self.assertNotIn('Duplicate As Maya Data', contextItemStrings)
        self.assertNotIn('Add Maya Reference', contextItemStrings)
        self.assertNotIn('AddReferenceOrPayload', contextItemStrings)
        self.assertNotIn('ClearAllReferencesOrPayloads', contextItemStrings)
        self.assertNotIn('ClearAllReferencesOrPayloads', contextItemStrings)

    def testSwitchVariantInLayer(self):
        """
        Test switching variant in layers: stronger, weaker, session.
        """
        contextItems = self.contextOps.getItems([])

        contextItemStrings = [c.item for c in contextItems]
        self.assertIn('Variant Sets', contextItemStrings)

        # Add an attribute in the session layer to see if it affects
        # switching variant.
        stage = self.ball35Prim.GetStage()
        stage.SetEditTarget(stage.GetSessionLayer())
        self.ball35Prim.GetAttribute("xformOpOrder").Set(Vt.TokenArray("translate"))

        stage.SetEditTarget(stage.GetRootLayer())

        # Initial shadingVariant is "Ball_8"
        contextItems = self.contextOps.getItems(
            ['Variant Sets', 'shadingVariant'])

        # Change variant in variant set.
        def shadingVariant():
            contextItems = self.contextOps.getItems(
                ['Variant Sets', 'shadingVariant'])

            for c in contextItems:
                if c.checked:
                    return c.item

        def shadingVariantOnPrim():
            variantSet = self.ball35Prim.GetVariantSet('shadingVariant')
            self.assertIsNotNone(variantSet)
            return variantSet.GetVariantSelection()

        self.assertEqual(shadingVariant(), 'Ball_8')

        cmd = self.contextOps.doOpCmd(
            ['Variant Sets', 'shadingVariant', 'Cue'])
        self.assertIsNotNone(cmd)

        ufeCmd.execute(cmd)

        self.assertEqual(shadingVariant(), 'Cue')
        self.assertEqual(shadingVariantOnPrim(), 'Cue')

        # Add a lower, weaker layer.
        rootLayer = stage.GetRootLayer()
        newLayerName = 'Layer_1'
        usdFormat = Sdf.FileFormat.FindByExtension('usd')
        subLayer = Sdf.Layer.New(usdFormat, newLayerName)
        rootLayer.subLayerPaths.append(subLayer.identifier)
        stage.SetEditTarget(subLayer)

        # Verify we cannot switch variant in a weaker layer.
        self.assertRaises(RuntimeError, lambda: self.contextOps.doOpCmd(
            ['Variant Sets', 'shadingVariant', 'Ball_8']).execute())

        # Verify the variant has not switched.
        self.assertEqual(shadingVariant(), 'Cue')
        self.assertEqual(shadingVariantOnPrim(), 'Cue')

        # Verify we can switch variant in Session Layer.
        stage.SetEditTarget(stage.GetSessionLayer())
        cmd = self.contextOps.doOpCmd(
            ['Variant Sets', 'shadingVariant', 'Ball_8'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)
        self.assertEqual(shadingVariant(), 'Ball_8')
        self.assertEqual(shadingVariantOnPrim(), 'Ball_8')

    def testDeactivateInLayer(self):
        """
        Test deactivate prim in layers: stronger, weaker, session.
        """
        self.assertTrue(self.ball35Prim.IsActive())
        stage = self.ball35Prim.GetStage()

        # Create a sub-layer of root to test weaker layers.
        rootLayer = stage.GetRootLayer()
        newLayerName = "Layer_1"
        usdFormat = Sdf.FileFormat.FindByExtension("usd")
        subLayer = Sdf.Layer.New(usdFormat, newLayerName)
        rootLayer.subLayerPaths.append(subLayer.identifier)

        def changeActivation(layer, activate, expectSuccess, undoIt=False):
            stage.SetEditTarget(layer)
            cmdName = "Toggle Active State"
            cmd = self.contextOps.doOpCmd([cmdName])
            self.assertIsNotNone(cmd)
            expectedActivation = bool(expectSuccess == activate)
            ufeCmd.execute(cmd)
            self.assertEqual(self.ball35Prim.IsActive(), expectedActivation)
            if undoIt:
                cmds.undo()
                # Note: when we expect failure, undo will do nothing, so in all
                #       cases we expect the opposite of the activate flag.
                self.assertEqual(self.ball35Prim.IsActive(), not activate)

        # Some boolean variables to make the code clearer below.
        doActivate = True
        doDeactivate = False
        shouldWork = True
        shouldFail = False
        thenUndo = True

        # Deactivate in root layer.
        changeActivation(stage.GetRootLayer(), doDeactivate, shouldWork)

        # Activate in root layer.
        changeActivation(stage.GetRootLayer(), doActivate, shouldWork, thenUndo)

        # Activate in session layer.
        changeActivation(stage.GetSessionLayer(), doActivate, shouldWork, thenUndo)

        # Activate in sub layer.
        changeActivation(subLayer, doActivate, shouldFail)

    def testInstanceableInLayer(self):
        """
        Test instanceable flag in layers: stronger, weaker, session.
        """
        self.assertTrue(self.ball35Prim.IsActive())
        stage = self.ball35Prim.GetStage()

        # Create a sub-layer of root to test weaker layers.
        rootLayer = stage.GetRootLayer()
        newLayerName = "Layer_1"
        usdFormat = Sdf.FileFormat.FindByExtension("usd")
        subLayer = Sdf.Layer.New(usdFormat, newLayerName)
        rootLayer.subLayerPaths.append(subLayer.identifier)

        def changeInstanceable(layer, makeInstanceable, expectSuccess, undoIt=False):
            stage.SetEditTarget(layer)
            cmdName = "Toggle Instanceable State"
            cmd = self.contextOps.doOpCmd([cmdName])
            self.assertIsNotNone(cmd)
            expectedInstanceable = bool(expectSuccess == makeInstanceable)
            ufeCmd.execute(cmd)
            self.assertEqual(self.ball35Prim.IsInstanceable(), expectedInstanceable)
            if undoIt:
                cmds.undo()
                # Note: when we expect failure, undo will do nothing, so in all
                #       cases we expect the opposite of the activate flag.
                self.assertEqual(self.ball35Prim.IsInstanceable(), not makeInstanceable)

        # Some boolean variables to make the code clearer below.
        doInstanceable = True
        doNonInstanceable = False
        shouldWork = True
        shouldFail = False
        thenUndo = True

        # Instanceable in root layer.
        changeInstanceable(stage.GetRootLayer(), doInstanceable, shouldWork)

        # Non-instanceable in root layer.
        changeInstanceable(
            stage.GetRootLayer(), doNonInstanceable, shouldWork, thenUndo
        )

        # Non-instanceable in session layer.
        changeInstanceable(
            stage.GetSessionLayer(), doNonInstanceable, shouldWork, thenUndo
        )

        # Non-instanceable in sub layer.
        changeInstanceable(subLayer, doNonInstanceable, shouldFail)

    def testDoOp(self):
        # Change visibility, undo / redo.
        cmd = self.contextOps.doOpCmd(['Toggle Visibility'])
        self.assertIsNotNone(cmd)

        attrs = ufe.Attributes.attributes(self.ball35Item)
        self.assertIsNotNone(attrs)
        visibility = attrs.attribute(UsdGeom.Tokens.visibility)
        self.assertIsNotNone(visibility)

        # Initially, Ball_35 has inherited visibility.
        self.assertEqual(visibility.get(), UsdGeom.Tokens.inherited)
        
        ufeCmd.execute(cmd)

        self.assertEqual(visibility.get(), UsdGeom.Tokens.invisible)
        
        cmds.undo()

        self.assertEqual(visibility.get(), UsdGeom.Tokens.inherited)

        cmds.redo()

        self.assertEqual(visibility.get(), UsdGeom.Tokens.invisible)
        
        cmds.undo()

        # Active / Deactivate Prim
        cmd = self.contextOps.doOpCmd(['Toggle Active State'])
        self.assertIsNotNone(cmd)

        # Initially, Ball_35 should be active.
        self.assertTrue(self.ball35Prim.IsActive())

        ufeCmd.execute(cmd)
        self.assertFalse(self.ball35Prim.IsActive())

        cmds.undo()
        self.assertTrue(self.ball35Prim.IsActive())

        cmds.redo()
        self.assertFalse(self.ball35Prim.IsActive())

        # Mark / Unmark Prim as Instanceable
        cmd = self.contextOps.doOpCmd(['Toggle Instanceable State'])
        self.assertIsNotNone(cmd)

        # Initially, Ball_35 should be not instanceable.
        self.assertFalse(self.ball35Prim.IsInstanceable())

        ufeCmd.execute(cmd)
        self.assertTrue(self.ball35Prim.IsInstanceable())

        cmds.undo()
        self.assertFalse(self.ball35Prim.IsInstanceable())

        cmds.redo()
        self.assertTrue(self.ball35Prim.IsInstanceable())

        # Change variant in variant set.
        def shadingVariant():
            contextItems = self.contextOps.getItems(
                ['Variant Sets', 'shadingVariant'])

            for c in contextItems:
                if c.checked:
                    return c.item

        self.assertEqual(shadingVariant(), 'Ball_8')

        cmd = self.contextOps.doOpCmd(
            ['Variant Sets', 'shadingVariant', 'Cue'])
        self.assertIsNotNone(cmd)

        ufeCmd.execute(cmd)

        self.assertEqual(shadingVariant(), 'Cue')

        cmds.undo()

        self.assertEqual(shadingVariant(), 'Ball_8')

        cmds.redo()

        self.assertEqual(shadingVariant(), 'Cue')

        cmds.undo()

    def testDoBulkOp(self):
        # The default setup selects a single prim (Ball_35) and creates context item
        # for it. We want multiple selection to make a bulk context.
        ballItems = {}
        ballPrims = {}
        for ball in [32, 33, 34]:
            ballPath = ufe.Path([
                mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
                usdUtils.createUfePathSegment("/Room_set/Props/Ball_%d" % ball)])
            ballItem = ufe.Hierarchy.createItem(ballPath)
            ballPrim = usdUtils.getPrimFromSceneItem(ballItem)
            ufe.GlobalSelection.get().append(ballItem)
            ballItems[ball] = ballItem
            ballPrims[ball] = ballPrim
        ballItems[35] = self.ball35Item
        ballPrims[35] = self.ball35Prim

        # Re-create a ContextOps interface for it.
        # Since the context item is in the selection, we get a bulk context.
        self.contextOps = ufe.ContextOps.contextOps(self.ball35Item)

        # Test Bulk Unload and Load with Descendants
        payloadFile = testUtils.getTestScene('twoSpheres', 'sphere.usda')

        def addPayLoads(payloadFile, ballPrims):
            for ballPrim in ballPrims.values():
                cmd = usdUfe.AddPayloadCommand(ballPrim, payloadFile, True)
                self.assertIsNotNone(cmd)

                # Verify state after add payload
                cmd.execute()
                self.assertTrue(ballPrim.HasPayload())
                self.assertTrue(ballPrim.IsLoaded())
    
        def verifyBulkPrimPayload(ballPrims, ifLoaded):
            for ballPrim in ballPrims.values():
                self.assertEqual(ballPrim.IsLoaded(), ifLoaded)

        addPayLoads(payloadFile, ballPrims)

        # Unload
        cmd = self.contextOps.doOpCmd(['Unload'])
        self.assertIsNotNone(cmd)
        self.assertIsInstance(cmd, ufe.CompositeUndoableCommand)

        ufeCmd.execute(cmd)
        verifyBulkPrimPayload(ballPrims, False)
        cmds.undo()
        verifyBulkPrimPayload(ballPrims, True)

        # Load with Descendants
        # Unload the payloads first, using the unload command
        ufeCmd.execute(cmd)
        cmd = self.contextOps.doOpCmd(['Load with Descendants'])
        self.assertIsNotNone(cmd)
        self.assertIsInstance(cmd, ufe.CompositeUndoableCommand)

        ufeCmd.execute(cmd)
        verifyBulkPrimPayload(ballPrims, True)
        cmds.undo()
        verifyBulkPrimPayload(ballPrims, False)


        # Change visility, undo/redo.
        cmd = self.contextOps.doOpCmd(['Make Invisible'])
        self.assertIsNotNone(cmd)
        self.assertIsInstance(cmd, ufe.CompositeUndoableCommand)

        # Get visibility attr for each selected ball.
        ballVisibility = {}
        for ball in ballItems:
            attrs = ufe.Attributes.attributes(ballItems[ball])
            self.assertIsNotNone(attrs)
            visibility = attrs.attribute(UsdGeom.Tokens.visibility)
            self.assertIsNotNone(visibility)
            ballVisibility[ball] = visibility

        def verifyBulkVisibility(ballVisibility, visValue):
            for ballVis in ballVisibility.values():
                self.assertEqual(ballVis.get(), visValue)

        # Initially all selected balls are inherited visibility.
        verifyBulkVisibility(ballVisibility, UsdGeom.Tokens.inherited)
        ufeCmd.execute(cmd)
        verifyBulkVisibility(ballVisibility, UsdGeom.Tokens.invisible)
        cmds.undo()
        verifyBulkVisibility(ballVisibility, UsdGeom.Tokens.inherited)
        cmds.redo()
        verifyBulkVisibility(ballVisibility, UsdGeom.Tokens.invisible)

        # Make Visible
        cmd = self.contextOps.doOpCmd(['Make Visible'])
        self.assertIsNotNone(cmd)
        self.assertIsInstance(cmd, ufe.CompositeUndoableCommand)

        # We left all the balls invisible from command above.
        ufeCmd.execute(cmd)
        verifyBulkVisibility(ballVisibility, UsdGeom.Tokens.inherited)
        cmds.undo()
        verifyBulkVisibility(ballVisibility, UsdGeom.Tokens.invisible)
        cmds.redo()
        verifyBulkVisibility(ballVisibility, UsdGeom.Tokens.inherited)

        # Deactivate Prim
        cmd = self.contextOps.doOpCmd(['Deactivate Prim'])
        self.assertIsNotNone(cmd)
        self.assertIsInstance(cmd, ufe.CompositeUndoableCommand)

        def verifyBulkPrimState(ballPrims, state):
            for ballPrim in ballPrims.values():
                self.assertEqual(ballPrim.IsActive(), state)

        # Initially, all selected balls should be Active.
        verifyBulkPrimState(ballPrims, True)
        ufeCmd.execute(cmd)
        verifyBulkPrimState(ballPrims, False)
        cmds.undo()
        verifyBulkPrimState(ballPrims, True)
        cmds.redo()
        verifyBulkPrimState(ballPrims, False)

        # Activate Prim
        cmd = self.contextOps.doOpCmd(['Activate Prim'])
        self.assertIsNotNone(cmd)
        self.assertIsInstance(cmd, ufe.CompositeUndoableCommand)

        # We left all the balls deactivated from command above.
        ufeCmd.execute(cmd)
        verifyBulkPrimState(ballPrims, True)
        cmds.undo()
        verifyBulkPrimState(ballPrims, False)
        cmds.redo()
        verifyBulkPrimState(ballPrims, True)

        # Mark as Instanceable
        cmd = self.contextOps.doOpCmd(['Mark as Instanceable'])
        self.assertIsNotNone(cmd)
        self.assertIsInstance(cmd, ufe.CompositeUndoableCommand)

        def verifyBulkPrimInstState(ballPrims, state):
            for ballPrim in ballPrims.values():
                self.assertEqual(ballPrim.IsInstanceable(), state)

        # Initially, all selected balls should be non-Instanceable.
        verifyBulkPrimInstState(ballPrims, False)
        ufeCmd.execute(cmd)
        verifyBulkPrimInstState(ballPrims, True)
        cmds.undo()
        verifyBulkPrimInstState(ballPrims, False)
        cmds.redo()
        verifyBulkPrimInstState(ballPrims, True)

        # Unmark as Instanceable
        cmd = self.contextOps.doOpCmd(['Unmark as Instanceable'])
        self.assertIsNotNone(cmd)
        self.assertIsInstance(cmd, ufe.CompositeUndoableCommand)

        # We left all the balls instanceable from command above.
        ufeCmd.execute(cmd)
        verifyBulkPrimInstState(ballPrims, False)
        cmds.undo()
        verifyBulkPrimInstState(ballPrims, True)
        cmds.redo()
        verifyBulkPrimInstState(ballPrims, False)

    def testAddNewPrim(self):
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create our UFE notification observer
        ufeObs = TestAddPrimObserver()
        ufe.Scene.addObserver(ufeObs)

        # Create a ContextOps interface for the proxy shape.
        proxyShapePath = ufe.Path([mayaUtils.createUfePathSegment(proxyShape)])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        # Add a new prim.
        cmd = contextOps.doOpCmd(['Add New Prim', 'Xform'])
        self.assertIsNotNone(cmd)
        ufeObs.reset()
        ufeCmd.execute(cmd)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertEqual(ufeObs.nbDeleteNotif(), 0)

        # The proxy shape should now have a single UFE child item.
        proxyShapehier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(proxyShapehier.hasChildren())
        self.assertEqual(len(proxyShapehier.children()), 1)

        # Add a new prim to the prim we just added.
        cmds.pickWalk(d='down')

        # Get the scene item from the UFE selection.
        snIter = iter(ufe.GlobalSelection.get())
        xformItem = next(snIter)

        # Create a ContextOps interface for it.
        contextOps = ufe.ContextOps.contextOps(xformItem)

        # Add a new prim.
        cmd = contextOps.doOpCmd(['Add New Prim', 'Xform'])
        self.assertIsNotNone(cmd)
        ufeObs.reset()
        ufeCmd.execute(cmd)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertEqual(ufeObs.nbDeleteNotif(), 0)

        # The xform prim should now have a single UFE child item.
        xformHier = ufe.Hierarchy.hierarchy(xformItem)
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 1)

        # Add another prim
        cmd = contextOps.doOpCmd(['Add New Prim', 'Capsule'])
        self.assertIsNotNone(cmd)
        ufeObs.reset()
        ufeCmd.execute(cmd)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertEqual(ufeObs.nbDeleteNotif(), 0)

        # The xform prim should now have two UFE child items.
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 2)

        # Undo will remove the new prim, meaning one less child.
        ufeObs.reset()
        cmds.undo()
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 1)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 0)
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)

        # Undo again will remove the first added prim, meaning no children.
        cmds.undo()
        self.assertFalse(xformHier.hasChildren())

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 0)
        self.assertEqual(ufeObs.nbDeleteNotif(), 2)

        cmds.redo()
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 1)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertEqual(ufeObs.nbDeleteNotif(), 2)

        cmds.redo()
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 2)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 2)
        self.assertEqual(ufeObs.nbDeleteNotif(), 2)

    def testUndoAddNewPrimCleanSessionLayer(self):
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(proxyShape).GetStage()

        # Create a ContextOps interface for the proxy shape.
        proxyShapePath = ufe.Path([mayaUtils.createUfePathSegment(proxyShape)])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        # Add a new prim.
        cmd = contextOps.doOpCmd(['Add New Prim', 'Xform'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        # The proxy shape should now have a single UFE child item.
        proxyShapehier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(proxyShapehier.hasChildren())
        self.assertEqual(len(proxyShapehier.children()), 1)

        # Add a new prim to the prim we just added.
        cmds.pickWalk(d='down')

        # Get the scene item from the UFE selection.
        snIter = iter(ufe.GlobalSelection.get())
        xformItem = next(snIter)
        xformPrim = usdUtils.getPrimFromSceneItem(xformItem)
        xformPath = xformPrim.GetPath()

        # Add data in the session layer.
        metadataName = 'instanceable'
        sessionLayer = stage.GetSessionLayer()
        with Usd.EditContext(stage, sessionLayer):
            xformPrim.SetMetadata(metadataName, True)

        self.assertTrue(xformPrim.HasAuthoredMetadata(metadataName))
        self.assertTrue(sessionLayer.GetPrimAtPath(xformPath))

        # Verify that after undo the sessin layer got cleaned.
        cmd.undo()
        self.assertFalse(sessionLayer.GetPrimAtPath(xformPath))

    def testAddNewPrimInWeakerLayer(self):
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create our UFE notification observer
        ufeObs = TestAddPrimObserver()
        ufe.Scene.addObserver(ufeObs)

        # Create a ContextOps interface for the proxy shape.
        proxyShapePath = ufe.Path([mayaUtils.createUfePathSegment(proxyShape)])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

         # Create a sub-layers SubLayerA.
        stage = mayaUsd.lib.GetPrim(proxyShape).GetStage()
        rootLayer = stage.GetRootLayer()
        subLayerA = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=False, addAnonymous="SubLayerA")[0]

        # Add a new prim.
        cmd = contextOps.doOpCmd(['Add New Prim', 'Xform'])
        self.assertIsNotNone(cmd)
        ufeObs.reset()
        ufeCmd.execute(cmd)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertEqual(ufeObs.nbDeleteNotif(), 0)

        # The proxy shape should now have a single UFE child item.
        proxyShapehier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(proxyShapehier.hasChildren())
        self.assertEqual(len(proxyShapehier.children()), 1)

        # Add a new prim to the prim we just added.
        cmds.pickWalk(d='down')

        # Get the scene item from the UFE selection.
        snIter = iter(ufe.GlobalSelection.get())
        xformItem = next(snIter)

        # Create a ContextOps interface for it.
        contextOps = ufe.ContextOps.contextOps(xformItem)

        # Add a new prim.
        cmd = contextOps.doOpCmd(['Add New Prim', 'Xform'])
        self.assertIsNotNone(cmd)
        ufeObs.reset()
        ufeCmd.execute(cmd)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertEqual(ufeObs.nbDeleteNotif(), 0)

        # The xform prim should now have a single UFE child item.
        xformHier = ufe.Hierarchy.hierarchy(xformItem)
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 1)

        # Set target to the weaker sub-layer.
        cmds.mayaUsdEditTarget(proxyShape, edit=True, editTarget=subLayerA)

        # Add another prim
        cmd = contextOps.doOpCmd(['Add New Prim', 'Capsule'])
        self.assertIsNotNone(cmd)
        ufeObs.reset()
        ufeCmd.execute(cmd)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertEqual(ufeObs.nbDeleteNotif(), 0)

        # The xform prim should now have two UFE child items.
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 2)

        # Undo will remove the new prim, meaning one less child.
        ufeObs.reset()
        cmds.undo()
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 1)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 0)
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)

        cmds.redo()
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 2)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)

    def testMaterialBinding(self):
        """This test builds a material using only Ufe pre-4.10 capabilities."""
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        cmd = contextOps.doOpCmd(['Add New Prim', 'Capsule'])
        ufeCmd.execute(cmd)
        cmd = contextOps.doOpCmd(['Add New Prim', 'Material'])
        ufeCmd.execute(cmd)

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(rootHier.hasChildren())
        self.assertEqual(len(rootHier.children()), 2)

        materialItem = rootHier.children()[-1]
        contextOps = ufe.ContextOps.contextOps(materialItem)

        cmd = contextOps.doOpCmd(['Add New Prim', 'Shader'])
        ufeCmd.execute(cmd)

        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        self.assertTrue(materialHier.hasChildren())
        self.assertEqual(len(materialHier.children()), 1)

        shaderItem = materialHier.children()[0]
        shaderAttrs = ufe.Attributes.attributes(shaderItem)

        self.assertTrue(shaderAttrs.hasAttribute("info:id"))
        shaderAttr = shaderAttrs.attribute("info:id")
        shaderAttr.set("ND_standard_surface_surfaceshader")

        # Now that we have a material, we can bind it on the capsule item even if incomplete
        capsuleItem = rootHier.children()[0]
        capsulePrim = usdUtils.getPrimFromSceneItem(capsuleItem)
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))

        contextOps = ufe.ContextOps.contextOps(capsuleItem)
        cmd = contextOps.doOpCmd(['Assign Material', '/Material1'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)
        self.assertTrue(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        capsuleBindAPI = UsdShade.MaterialBindingAPI(capsulePrim)
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Material1"))

        cmds.undo()
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        cmds.redo()
        self.assertTrue(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Material1"))

        cmd = contextOps.doOpCmd(['Unassign Material'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        self.assertTrue(capsuleBindAPI.GetDirectBinding().GetMaterialPath().isEmpty)
        cmds.undo()
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Material1"))
        cmds.redo()
        self.assertTrue(capsuleBindAPI.GetDirectBinding().GetMaterialPath().isEmpty)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testMaterialCreationInLockedLayer(self):
        """This test creates a material in a locked layer. This should fail but not crash."""
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        proxyShape, stage = mayaUtils.createProxyAndStage()

        # Create a cube prim without material.
        cubeUsdPathStr = '/MyCube'
        cubePrim = stage.DefinePrim(cubeUsdPathStr, 'Cube')
        self.assertFalse(cubePrim.HasAPI(UsdShade.MaterialBindingAPI))

        # Create a sub-layer, target it and lock it.
        subLayer = usdUtils.addNewLayerToStage(stage, anonymous=True)
        stage.SetEditTarget(subLayer)
        subLayer.SetPermissionToEdit(False)

        # try to create a material on the cube prim.
        cubeSceneItem = ufeUtils.createUfeSceneItem(proxyShape, cubeUsdPathStr)
        contextOps = ufe.ContextOps.contextOps(cubeSceneItem)
        cmdPS = contextOps.doOpCmd(['Assign New Material', 'USD', 'UsdPreviewSurface'])
        self.assertIsNotNone(cmdPS)
        ufeCmd.execute(cmdPS)

        # Verify the command filed due to the lock, but did not crash.
        self.assertFalse(cubePrim.HasAPI(UsdShade.MaterialBindingAPI))

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testMaterialCreationForSingleObject(self):
        """This test builds a material using contextOps capabilities."""
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        cmd = contextOps.doOpCmd(['Add New Prim', 'Capsule'])
        ufeCmd.execute(cmd)

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(rootHier.hasChildren())
        self.assertEqual(len(rootHier.children()), 1)

        capsuleItem = rootHier.children()[-1]
        contextOps = ufe.ContextOps.contextOps(capsuleItem)

        capsulePrim = usdUtils.getPrimFromSceneItem(capsuleItem)
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))

        cmdPS = contextOps.doOpCmd(['Assign New Material', 'USD', 'UsdPreviewSurface'])
        self.assertIsNotNone(cmdPS)
        ufeCmd.execute(cmdPS)

        # Complex command. We should now have a fully working preview surface material bound to
        # the capsule prim:
        def checkMaterial(self, rootHier, rootHierChildren, numMat, idx, matType, context, shaderOutputName, scope="/mtl"):
            self.assertTrue(rootHier.hasChildren())
            self.assertEqual(len(rootHier.children()), rootHierChildren)

            def checkItem(self, item, type, path):
                self.assertEqual(item.nodeType(), type)            
                prim = usdUtils.getPrimFromSceneItem(item)
                self.assertEqual(prim.GetPath(), Sdf.Path(path))

            scopeItem = rootHier.children()[-1]
            checkItem(self, scopeItem, "Scope", "{0}".format(scope))

            scopeHier = ufe.Hierarchy.hierarchy(scopeItem)
            self.assertTrue(scopeHier.hasChildren())
            self.assertEqual(len(scopeHier.children()), numMat)
            materialItem = scopeHier.children()[idx]
            checkItem(self, materialItem, "Material", "{0}/{1}1".format(scope, matType))

            if USD_HAS_MATERIALX_PLUGIN and Usd.GetVersion() >= (0, 25,2) and matType == "standard_surface":
                # Verify we have a MaterialX version schema:
                self.assertTrue(UsdMtlx.MaterialXConfigAPI(usdUtils.getPrimFromSceneItem(materialItem)))

            # Binding and selection are always on the last item:
            if (numMat - 1 == idx):
                self.assertTrue(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
                self.assertEqual(UsdShade.MaterialBindingAPI(capsulePrim).GetDirectBinding().GetMaterialPath(),
                                Sdf.Path("{0}/{1}1".format(scope, matType)))
                selection = ufe.GlobalSelection.get()
                self.assertEqual(len(selection), 1)
                self.assertEqual(selection.front().nodeName(), "{0}1".format(matType))
                self.assertEqual(selection.front().nodeType(), "Shader")


            hier = ufe.Hierarchy.hierarchy(materialItem)
            self.assertTrue(hier.hasChildren())
            self.assertEqual(len(hier.children()), 1)
            shaderItem = hier.children()[0]
            checkItem(self, shaderItem, "Shader", "{0}/{1}1/{1}1".format(scope, matType))

            materialPrim = UsdShade.Material(usdUtils.getPrimFromSceneItem(materialItem))
            self.assertTrue(materialPrim)

            surfaceOutput = materialPrim.GetSurfaceOutput(context)
            self.assertTrue(surfaceOutput)

            sourceInfos, invalidPaths = surfaceOutput.GetConnectedSources()
            self.assertEqual(len(sourceInfos), 1)
            self.assertEqual(sourceInfos[0].source.GetPath(), Sdf.Path("{0}/{1}1/{1}1".format(scope, matType)))
            self.assertEqual(sourceInfos[0].sourceName, shaderOutputName)

        checkMaterial(self, rootHier, 2, 1, 0, "UsdPreviewSurface", "", "surface")

        cmdSS = contextOps.doOpCmd(['Assign New Material', 'MaterialX', 'ND_standard_surface_surfaceshader'])
        self.assertIsNotNone(cmdSS)
        ufeCmd.execute(cmdSS)

        checkMaterial(self, rootHier, 2, 2, 0, "UsdPreviewSurface", "", "surface")
        checkMaterial(self, rootHier, 2, 2, 1, "standard_surface", "mtlx", "out")

        cmds.undo()

        checkMaterial(self, rootHier, 2, 1, 0, "UsdPreviewSurface", "", "surface")

        cmds.redo()

        checkMaterial(self, rootHier, 2, 2, 0, "UsdPreviewSurface", "", "surface")
        checkMaterial(self, rootHier, 2, 2, 1, "standard_surface", "mtlx", "out")

        cmds.undo()
        cmds.undo()

        # Not even a mtl scope:
        self.assertEqual(len(rootHier.children()), 1)
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))

        cmds.redo()

        checkMaterial(self, rootHier, 2, 1, 0, "UsdPreviewSurface", "", "surface")

        with testUtils.TemporaryEnvironmentVariable("MAYAUSD_MATERIALS_SCOPE_NAME", "test_scope"):
            ufeCmd.execute(cmdSS)
            checkMaterial(self, rootHier, 3, 1, 0, "standard_surface", "mtlx", "out", "/test_scope")

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testMaterialCreationForMultipleObjects(self):
        """This test creates a single shared material for multiple objects using contextOps capabilities."""
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        # Create multiple objects to test with. 
        cmd = contextOps.doOpCmd(['Add New Prim', 'Capsule'])
        ufeCmd.execute(cmd)
        cmd = contextOps.doOpCmd(['Add New Prim', 'Cube'])
        ufeCmd.execute(cmd)
        cmd = contextOps.doOpCmd(['Add New Prim', 'Sphere'])
        ufeCmd.execute(cmd)

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(rootHier.hasChildren())
        self.assertEqual(len(rootHier.children()), 3)

        capsuleItem = rootHier.children()[-3]
        cubeItem = rootHier.children()[-2]
        sphereItem = rootHier.children()[-1]

        capsulePrim = usdUtils.getPrimFromSceneItem(capsuleItem)
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        cubePrim = usdUtils.getPrimFromSceneItem(cubeItem)
        self.assertFalse(cubePrim.HasAPI(UsdShade.MaterialBindingAPI))
        spherePrim = usdUtils.getPrimFromSceneItem(sphereItem)
        self.assertFalse(spherePrim.HasAPI(UsdShade.MaterialBindingAPI))

        # Select all three objects.
        ufe.GlobalSelection.get().append(capsuleItem)
        ufe.GlobalSelection.get().append(cubeItem)
        ufe.GlobalSelection.get().append(sphereItem)

        # Apply the new material one of the selected objects, so we are operating in
        # bulk mode. All the selected objects should receive the new material binding.
        contextOps = ufe.ContextOps.contextOps(capsuleItem)

        # Create a new material and apply it to our cube, sphere and capsule objects.
        cmdPS = contextOps.doOpCmd(['Assign New Material', 'USD', 'UsdPreviewSurface'])
        self.assertIsNotNone(cmdPS)
        ufeCmd.execute(cmdPS)

        # Complex command. We should now have a fully working preview surface material bound to
        # the capsule, sphere and cube prims:
        def checkMaterial(self, rootHier, rootHierChildren, numMat, selectedObjCount, idx, matType, context, shaderOutputName, scope="/mtl"):
            self.assertTrue(rootHier.hasChildren())
            self.assertEqual(len(rootHier.children()), rootHierChildren)

            def checkItem(self, item, type, path):
                self.assertEqual(item.nodeType(), type)
                prim = usdUtils.getPrimFromSceneItem(item)
                self.assertEqual(prim.GetPath(), Sdf.Path(path))

            scopeItem = rootHier.children()[-1]
            checkItem(self, scopeItem, "Scope", "{0}".format(scope))

            scopeHier = ufe.Hierarchy.hierarchy(scopeItem)
            self.assertTrue(scopeHier.hasChildren())
            self.assertEqual(len(scopeHier.children()), numMat)
            materialItem = scopeHier.children()[idx]
            checkItem(self, materialItem, "Material", "{0}/{1}1".format(scope, matType))

            # Binding and selection are always on the last item:
            if (numMat - 1 == idx):
                self.assertTrue(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
                self.assertEqual(UsdShade.MaterialBindingAPI(capsulePrim).GetDirectBinding().GetMaterialPath(),
                                Sdf.Path("{0}/{1}1".format(scope, matType)))

                self.assertTrue(cubePrim.HasAPI(UsdShade.MaterialBindingAPI))
                self.assertEqual(UsdShade.MaterialBindingAPI(cubePrim).GetDirectBinding().GetMaterialPath(),
                                Sdf.Path("{0}/{1}1".format(scope, matType)))

                self.assertTrue(spherePrim.HasAPI(UsdShade.MaterialBindingAPI))
                self.assertEqual(UsdShade.MaterialBindingAPI(spherePrim).GetDirectBinding().GetMaterialPath(),
                                Sdf.Path("{0}/{1}1".format(scope, matType)))

                if (selectedObjCount == 1):
                    selection = ufe.GlobalSelection.get()
                    self.assertEqual(len(selection), selectedObjCount)
                    self.assertEqual(selection.front().nodeName(), "{0}1".format(matType))
                    self.assertEqual(selection.front().nodeType(), "Shader")

            hier = ufe.Hierarchy.hierarchy(materialItem)
            self.assertTrue(hier.hasChildren())
            self.assertEqual(len(hier.children()), 1)
            shaderItem = hier.children()[0]
            checkItem(self, shaderItem, "Shader", "{0}/{1}1/{1}1".format(scope, matType))

            materialPrim = UsdShade.Material(usdUtils.getPrimFromSceneItem(materialItem))
            self.assertTrue(materialPrim)

            surfaceOutput = materialPrim.GetSurfaceOutput(context)
            self.assertTrue(surfaceOutput)

            sourceInfos, invalidPaths = surfaceOutput.GetConnectedSources()
            self.assertEqual(len(sourceInfos), 1)
            self.assertEqual(sourceInfos[0].source.GetPath(), Sdf.Path("{0}/{1}1/{1}1".format(scope, matType)))
            self.assertEqual(sourceInfos[0].sourceName, shaderOutputName)

        checkMaterial(self, rootHier, 4, 1, 1, 0, "UsdPreviewSurface", "", "surface")

        # Re-select our multiple objects so that we can repeat the test with another material.
        ufe.GlobalSelection.get().append(capsuleItem)
        ufe.GlobalSelection.get().append(cubeItem)
        ufe.GlobalSelection.get().append(sphereItem)

        cmdSS = contextOps.doOpCmd(['Assign New Material', 'MaterialX', 'ND_standard_surface_surfaceshader'])
        self.assertIsNotNone(cmdSS)
        ufeCmd.execute(cmdSS)

        checkMaterial(self, rootHier, 4, 2, 1, 0, "UsdPreviewSurface", "", "surface")
        checkMaterial(self, rootHier, 4, 2, 1, 1, "standard_surface", "mtlx", "out")

        cmds.undo()

        checkMaterial(self, rootHier, 4, 1, 3, 0, "UsdPreviewSurface", "", "surface")

        cmds.redo()

        checkMaterial(self, rootHier, 4, 2, 1, 0, "UsdPreviewSurface", "", "surface")
        checkMaterial(self, rootHier, 4, 2, 1, 1, "standard_surface", "mtlx", "out")

        cmds.undo()
        cmds.undo()

        # Not even a mtl scope:
        self.assertEqual(len(rootHier.children()), 3)
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        self.assertFalse(cubePrim.HasAPI(UsdShade.MaterialBindingAPI))
        self.assertFalse(spherePrim.HasAPI(UsdShade.MaterialBindingAPI))

        cmds.redo()

        checkMaterial(self, rootHier, 4, 1, 1, 0, "UsdPreviewSurface", "", "surface")

        with testUtils.TemporaryEnvironmentVariable("MAYAUSD_MATERIALS_SCOPE_NAME", "test_scope"):
            ufeCmd.execute(cmdSS)
            checkMaterial(self, rootHier, 5, 1, 1, 0, "standard_surface", "mtlx", "out", "/test_scope")

        # Unassign Material works on bulk
        cmd = contextOps.doOpCmd(['Unassign Material'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        self.assertTrue(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        capsuleBindAPI = UsdShade.MaterialBindingAPI(capsulePrim)
        self.assertTrue(capsuleBindAPI.GetDirectBinding().GetMaterialPath().isEmpty)
        self.assertTrue(cubePrim.HasAPI(UsdShade.MaterialBindingAPI))
        cubeBindAPI = UsdShade.MaterialBindingAPI(cubePrim)
        self.assertTrue(cubeBindAPI.GetDirectBinding().GetMaterialPath().isEmpty)
        self.assertTrue(spherePrim.HasAPI(UsdShade.MaterialBindingAPI))
        sphereBindAPI = UsdShade.MaterialBindingAPI(spherePrim)
        self.assertTrue(sphereBindAPI.GetDirectBinding().GetMaterialPath().isEmpty)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testMaterialCreationScopeName(self):
        """This test verifies that materials get created in the correct scope."""
        cmds.file(new=True, force=True)

        # Helper function to create a new proxy shape.
        def createProxyShape():
            proxyShapePathString = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
            proxyShapePath = ufe.PathString.path(proxyShapePathString)
            return ufe.Hierarchy.createItem(proxyShapePath)

        # Helper function to add a named new prim to a proxy shape.
        def addNewPrim(proxyShape, primType, name = None):
            ufe.ContextOps.contextOps(proxyShape).doOp(["Add New Prim", primType])
            primItem = ufe.Hierarchy.hierarchy(proxyShape).children()[-1]
            assert primItem

            if name != None:
                primItem = ufe.SceneItemOps.sceneItemOps(primItem).renameItem(ufe.PathComponent(name))
                assert primItem
                assert ufe.PathString.string(primItem.path()) == ufe.PathString.string(proxyShape.path()) + ",/" + name
            
            return primItem

        # Helper function that adds a sphere prim to a proxy shape and assigns it a new material.
        def createMaterial(proxyShape):
            ufe.ContextOps.contextOps(proxyShape).doOp(['Add New Prim', 'Sphere'])
            sphereItem = ufe.Hierarchy.hierarchy(proxyShape).children()[-1]
            ufe.ContextOps.contextOps(sphereItem).doOp(['Assign New Material', 'USD', 'UsdPreviewSurface'])

        # Default names.
        materialsScopeName = "mtl"
        materialName = "UsdPreviewSurface1"

        # Case 1: Empty stage.
        # The new material should get created in a new scope named "mtl".
        proxyShape = createProxyShape()
        proxyShapePath = ufe.PathString.string(proxyShape.path())
        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "/" + materialName
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

        # Case 2: A scope named "mtl" exists.
        # The new material should get created in the already existing scope named "mtl".
        proxyShape = createProxyShape()
        proxyShapePath = ufe.PathString.string(proxyShape.path())
        addNewPrim(proxyShape, "Scope", materialsScopeName)
        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "/" + materialName
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

        # Case 3: A scope named "mtl1" exists.
        # The new material should get created in a new scope named "mtl".
        proxyShape = createProxyShape()
        proxyShapePath = ufe.PathString.string(proxyShape.path())
        addNewPrim(proxyShape, "Scope", materialsScopeName + "1")
        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "/" + materialName
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

        # Case 4: A non-scope object named "mtl" exists.
        # The new material should get created in a new scope named "mtl1".
        proxyShape = createProxyShape()
        proxyShapePath = ufe.PathString.string(proxyShape.path())
        addNewPrim(proxyShape, "Def", materialsScopeName)
        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "1/" + materialName
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

        # Case 5: A non-scope object named "mtl" exists and a scope named "mtl1" exists.
        # The new material should get created in the existing scope named "mtl1".
        proxyShape = createProxyShape()
        proxyShapePath = ufe.PathString.string(proxyShape.path())
        addNewPrim(proxyShape, "Def", materialsScopeName)
        addNewPrim(proxyShape, "Scope", materialsScopeName +"1")
        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "1/" + materialName
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

        # Case 6: A non-scope object named "mtl" exists and a scope named "mtl2" exists.
        # The new material should get created in a new scope named "mtl3".
        # This follows normal Maya naming standard where it increments the largest numerical
        # suffix (even if there are gaps).
        proxyShape = createProxyShape()
        proxyShapePath = ufe.PathString.string(proxyShape.path())
        addNewPrim(proxyShape, "Def", materialsScopeName)
        addNewPrim(proxyShape, "Scope", materialsScopeName + "2")
        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "3/" + materialName
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

        # Case 7: A non-scope object named "mtl" exists and a scope named "mtlBingBong" exists.
        # The new material should get created in a new scope named "mtl1".
        proxyShape = createProxyShape()
        proxyShapePath = ufe.PathString.string(proxyShape.path())
        addNewPrim(proxyShape, "Def", materialsScopeName)
        addNewPrim(proxyShape, "Scope", materialsScopeName + "BingBong")
        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "1/" + materialName
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

        # Case 8: A non-scope object named "mtl" exists and multiple scopes starting in "mtl" exist.
        # The new material should get created in a scope named "mtl1".
        proxyShape = createProxyShape()
        proxyShapePath = ufe.PathString.string(proxyShape.path())
        addNewPrim(proxyShape, "Def", materialsScopeName)
        
        scopeNamePostfixes = ["BingBong", "XingXong", "Arnold", "1337"]
        for scopeNamePostfix in scopeNamePostfixes:
            addNewPrim(proxyShape, "Scope", materialsScopeName + scopeNamePostfix)

        # Again here follow the normal Maya naming standard which increments the largest
        # numerical suffix (1337).
        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "1338/" + materialName
        print(expectedPath)
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

        # Case 9: Multiple non-scope object named "mtl", "mtl1", ..., "mtl3" exists and multiple 
        #         scopes starting in "mtl" exist. An object named "mtl4" does not exist.
        # The new material should get created in a new scope named "mtl4".
        proxyShape = createProxyShape()
        proxyShapePath = ufe.PathString.string(proxyShape.path())
        
        defNamePostfixes = ["", "1", "2", "3"]
        for defNamePostfix in defNamePostfixes:
            addNewPrim(proxyShape, "Def", materialsScopeName + defNamePostfix)

        scopeNamePostfixes = ["BingBong", "XingXong", "Arnold", "1337"]
        for scopeNamePostfix in scopeNamePostfixes:
            addNewPrim(proxyShape, "Scope", materialsScopeName + scopeNamePostfix)

        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "1338/" + materialName
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

        # Case 10: Multiple non-scope object named "mtl", "mtl1", ..., "mtl3" exists and multiple 
        #          scopes starting in "mtl" exist. A scope named "mtl4" exists.
        # The new material should get created in the existing scope named "mtl4".
        proxyShape = createProxyShape()
        proxyShapePath = ufe.PathString.string(proxyShape.path())
        
        defNamePostfixes = ["", "1", "2", "3"]
        for defNamePostfix in defNamePostfixes:
            addNewPrim(proxyShape, "Def", materialsScopeName + defNamePostfix)

        scopeNamePostfixes = ["BingBong", "XingXong", "Arnold", "1337", "4", "5"]
        for scopeNamePostfix in scopeNamePostfixes:
            addNewPrim(proxyShape, "Scope", materialsScopeName + scopeNamePostfix)

        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "4/" + materialName
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

        # Case 11: A material got created in a scope called "mtl". Afterwards  the scope was renamed
        #          to "mtl4". Now a new material gets created.
        # The new material should get created in a new scope named "mtl".
        proxyShape = createProxyShape()
        proxyShapePath = ufe.PathString.string(proxyShape.path())

        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "/" + materialName
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

        scopeItem = ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath).pop())
        scopeItem = ufe.SceneItemOps.sceneItemOps(scopeItem).renameItem(ufe.PathComponent(materialsScopeName + "4"))
        assert scopeItem
        assert ufe.PathString.string(scopeItem.path()) == proxyShapePath + ",/" + materialsScopeName + "4"
        assert not ufe.Hierarchy.createItem(ufe.PathString.path(proxyShapePath + ",/" + materialsScopeName))

        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "/" + materialName
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

        # Case 12: A material got created in a scope called "mtl". Afterwards  the scope was renamed
        #          to "mtlBingBong". Now a new material gets created.
        # The new material should get created in a new scope named "mtl".
        proxyShape = createProxyShape()
        proxyShapePath = ufe.PathString.string(proxyShape.path())

        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "/" + materialName
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

        scopeItem = ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath).pop())
        scopeItem = ufe.SceneItemOps.sceneItemOps(scopeItem).renameItem(ufe.PathComponent(materialsScopeName + "BingBong"))
        assert scopeItem
        assert ufe.PathString.string(scopeItem.path()) == proxyShapePath + ",/" + materialsScopeName + "BingBong"
        assert not ufe.Hierarchy.createItem(ufe.PathString.path(proxyShapePath + ",/" + materialsScopeName))

        createMaterial(proxyShape)
        expectedPath = proxyShapePath + ",/" + materialsScopeName + "/" + materialName
        assert ufe.Hierarchy.createItem(ufe.PathString.path(expectedPath))

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testAddMaterialToScope(self):
        """This test adds a new material to the material scope."""
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        # Create multiple objects to test with. 
        cmd = contextOps.doOpCmd(['Add New Prim', 'Capsule'])
        ufeCmd.execute(cmd)

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(rootHier.hasChildren())
        self.assertEqual(len(rootHier.children()), 1)

        capsuleItem = rootHier.children()[0]

        capsulePrim = usdUtils.getPrimFromSceneItem(capsuleItem)
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))

        contextOps = ufe.ContextOps.contextOps(capsuleItem)

        # Create a new material and apply it to our cube, sphere and capsule objects.
        cmdPS = contextOps.doOpCmd(['Assign New Material', 'USD', 'UsdPreviewSurface'])
        self.assertIsNotNone(cmdPS)
        ufeCmd.execute(cmdPS)

        scopeItem = rootHier.children()[-1]
        scopeHier = ufe.Hierarchy.hierarchy(scopeItem)
        self.assertTrue(scopeHier.hasChildren())
        self.assertEqual(len(scopeHier.children()), 1)

        scopeOps = ufe.ContextOps.contextOps(scopeItem)
        cmdAddSS = scopeOps.doOpCmd(['Add New Material', 'MaterialX', 'ND_standard_surface_surfaceshader'])
        ufeCmd.execute(cmdAddSS)

        # Should now be two materials in the scope.
        self.assertEqual(len(scopeHier.children()), 2)

        cmds.undo()

        self.assertEqual(len(scopeHier.children()), 1)

        cmds.redo()

        self.assertEqual(len(scopeHier.children()), 2)

        newMatItem = scopeHier.children()[-1]

        if USD_HAS_MATERIALX_PLUGIN and Usd.GetVersion() >= (0, 25,2):
            # Verify we have a MaterialX version schema:
            self.assertTrue(UsdMtlx.MaterialXConfigAPI(usdUtils.getPrimFromSceneItem(newMatItem)))

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(newMatItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(newMatItem)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        mxConn = conns[0]
        self.assertEqual(ufe.PathString.string(mxConn.src.path), "|stage1|stageShape1,/mtl/standard_surface1/standard_surface1")
        self.assertEqual(mxConn.src.name, "outputs:out")
        self.assertEqual(ufe.PathString.string(mxConn.dst.path), "|stage1|stageShape1,/mtl/standard_surface1")
        self.assertEqual(mxConn.dst.name, "outputs:mtlx:surface")

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testMaterialBindingWithNodeDefHandler(self):
        """In this test we will go as far as possible towards creating and binding a working
           material using only Ufe and Maya commands (for full undo capabilities). It is locked
           at what Ufe 0.4.10 offers."""
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        cmd = contextOps.doOpCmd(['Add New Prim', 'Capsule'])
        ufeCmd.execute(cmd)
        cmd = contextOps.doOpCmd(['Add New Prim', 'Material'])
        ufeCmd.execute(cmd)

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(rootHier.hasChildren())
        self.assertEqual(len(rootHier.children()), 2)

        materialItem = rootHier.children()[-1]
        contextOps = ufe.ContextOps.contextOps(materialItem)

        shaderName = "ND_standard_surface_surfaceshader"
        surfDef = ufe.NodeDef.definition(materialItem.runTimeId(), shaderName)
        cmd = surfDef.createNodeCmd(materialItem, ufe.PathComponent("Red1"))
        ufeCmd.execute(cmd)
        shaderItem = cmd.insertedChild
        shaderAttrs = ufe.Attributes.attributes(shaderItem)

        self.assertTrue(shaderAttrs.hasAttribute("info:id"))
        self.assertEqual(shaderAttrs.attribute("info:id").get(), shaderName)
        self.assertEqual(ufe.PathString.string(shaderItem.path()), "|stage1|stageShape1,/Material1/Red1")
        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        self.assertTrue(materialHier.hasChildren())

        cmds.undo()
        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        self.assertFalse(materialHier.hasChildren())

        cmds.redo()
        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        self.assertTrue(materialHier.hasChildren())
        shaderItem = cmd.insertedChild
        shaderAttrs = ufe.Attributes.attributes(shaderItem)

        # Now that we have a material, we can bind it on the capsule item even if incomplete
        capsuleItem = rootHier.children()[0]
        capsulePrim = usdUtils.getPrimFromSceneItem(capsuleItem)
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))

        contextOps = ufe.ContextOps.contextOps(capsuleItem)
        cmd = contextOps.doOpCmd(['Assign Material', '/Material1'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)
        self.assertTrue(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        capsuleBindAPI = UsdShade.MaterialBindingAPI(capsulePrim)
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Material1"))

        cmds.undo()
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        cmds.redo()
        self.assertTrue(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Material1"))

        cmd = contextOps.doOpCmd(['Unassign Material'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        self.assertTrue(capsuleBindAPI.GetDirectBinding().GetMaterialPath().isEmpty)
        cmds.undo()
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Material1"))
        cmds.redo()
        self.assertTrue(capsuleBindAPI.GetDirectBinding().GetMaterialPath().isEmpty)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testMaterialBindingToSelection(self):
        """Exercising the bind to selection context menu option."""
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyShapeItem = ufeUtils.createItem(proxyShape)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        for primName in ['Capsule', 'Sphere', 'Material', 'Material']:
            cmd = contextOps.doOpCmd(['Add New Prim', primName])
            ufeCmd.execute(cmd)

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(rootHier.hasChildren())
        self.assertEqual(len(rootHier.children()), 4)

        # Add the capsule and sphere to the global selection:
        ufe.GlobalSelection.get().clear()
        capsuleItem = rootHier.children()[0]
        ufe.GlobalSelection.get().append(capsuleItem)
        sphereItem = rootHier.children()[1]
        ufe.GlobalSelection.get().append(sphereItem)

        def allHaveMaterial(self, items, materialName):
            for item in items:
                prim = usdUtils.getPrimFromSceneItem(item)
                self.assertTrue(prim.HasAPI(UsdShade.MaterialBindingAPI))
                bindAPI = UsdShade.MaterialBindingAPI(prim)
                self.assertEqual(bindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path(materialName))

        def noneHaveMaterial(self, items):
            for item in items:
                prim = usdUtils.getPrimFromSceneItem(item)
                self.assertFalse(prim.HasAPI(UsdShade.MaterialBindingAPI))

        contextOps = ufe.ContextOps.contextOps(rootHier.children()[2])
        cmd = contextOps.doOpCmd(['Assign Material to Selection',])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)
        allHaveMaterial(self, [capsuleItem, sphereItem], "/Material1")
        cmds.undo()
        noneHaveMaterial(self, [capsuleItem, sphereItem])
        cmds.redo()
        allHaveMaterial(self, [capsuleItem, sphereItem], "/Material1")

        # Test that undo restores previous material:
        contextOps = ufe.ContextOps.contextOps(rootHier.children()[3])
        cmd = contextOps.doOpCmd(['Assign Material to Selection',])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)
        allHaveMaterial(self, [capsuleItem, sphereItem], "/Material2")
        cmds.undo()
        allHaveMaterial(self, [capsuleItem, sphereItem], "/Material1")
        cmds.redo()
        allHaveMaterial(self, [capsuleItem, sphereItem], "/Material2")

    def testAddNewPrimWithDelete(self):
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyShapePath = ufe.Path([mayaUtils.createUfePathSegment(proxyShape)])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        # Add a new Xform prim.
        cmd = contextOps.doOpCmd(['Add New Prim', 'Xform'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        # The proxy shape should now have a single UFE child item.
        proxyShapehier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(proxyShapehier.hasChildren())
        self.assertEqual(len(proxyShapehier.children()), 1)

        # Using UFE, delete this new prim (which doesn't actually delete it but
        # instead makes it inactive). In Maya 2023 and greater, delete really
        # deletes so deactivate instead to make the test act the same for all
        # versions.
        cmds.pickWalk(d='down')
        if mayaUtils.mayaMajorVersion() >= 2023:
            ufeItem = ufe.GlobalSelection.get().front()
            item = usdUtils.getPrimFromSceneItem(ufeItem)
            item.SetActive(False)
        else:
            cmds.delete()

        # The proxy shape should now have no UFE child items (since we skip inactive
        # when returning the children list) but hasChildren still reports true in
        # UFE version before 0.4.4 for inactive to allow the caller to do conditional
        # inactive filtering, so we test that hasChildren is true for those versions.
        if (ufeUtils.ufeFeatureSetVersion() >= 4):
            self.assertFalse(proxyShapehier.hasChildren())
        else:
            self.assertTrue(proxyShapehier.hasChildren())
        self.assertEqual(len(proxyShapehier.children()), 0)

        # Add another Xform prim (which should get a unique name taking into
        # account the prim we just made inactive).
        cmd = contextOps.doOpCmd(['Add New Prim', 'Xform'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        # The proxy shape should now have a single UFE child item.
        self.assertTrue(proxyShapehier.hasChildren())
        self.assertEqual(len(proxyShapehier.children()), 1)

    def testLoadAndUnload(self):
        '''
        Tests the working set management contextOps "Load", "Load with
        Descendants", and "Unload".
        '''
        proxyShapePathSegment = mayaUtils.createUfePathSegment(
            '|transform1|proxyShape1')

        propsPath = ufe.Path([
            proxyShapePathSegment,
            usdUtils.createUfePathSegment('/Room_set/Props')])
        propsItem = ufe.Hierarchy.createItem(propsPath)
        ball1Path = ufe.Path([
            proxyShapePathSegment,
            usdUtils.createUfePathSegment('/Room_set/Props/Ball_1')])
        ball1Item = ufe.Hierarchy.createItem(ball1Path)
        ball15Path = ufe.Path([
            proxyShapePathSegment,
            usdUtils.createUfePathSegment('/Room_set/Props/Ball_15')])
        ball15Item = ufe.Hierarchy.createItem(ball15Path)

        def _validateLoadAndUnloadItems(hierItem, itemStrings):
            ALL_ITEM_STRINGS = set([
                'Load',
                'Load with Descendants',
                'Unload'
            ])

            expectedItemStrings = set(itemStrings or [])
            expectedAbsentItemStrings = ALL_ITEM_STRINGS - expectedItemStrings

            contextOps = ufe.ContextOps.contextOps(hierItem)
            contextItems = contextOps.getItems([])
            contextItemStrings = [c.item for c in contextItems]

            for itemString in expectedItemStrings:
                self.assertIn(itemString, contextItemStrings)

            for itemString in expectedAbsentItemStrings:
                self.assertNotIn(itemString, contextItemStrings)

        # The stage is fully loaded, so all items should have "Unload" items.
        _validateLoadAndUnloadItems(propsItem, ['Unload'])
        _validateLoadAndUnloadItems(ball1Item, ['Unload'])
        _validateLoadAndUnloadItems(ball15Item, ['Unload'])

        # The mesh prim path under each Ball asset prim has nothing loadable at
        # or below it, so it should not have any load or unload items.
        ball15meshPath = ufe.Path([
            proxyShapePathSegment,
            usdUtils.createUfePathSegment('/Room_set/Props/Ball_15/mesh')])
        ball15meshItem = ufe.Hierarchy.createItem(ball15meshPath)
        _validateLoadAndUnloadItems(ball15meshItem, [])

        # Unload Ball_1.
        contextOps = ufe.ContextOps.contextOps(ball1Item)
        cmd = contextOps.doOpCmd(['Unload'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        # Only Ball_1 should have been unloaded, and since it has a payload, it
        # should now have "Load" and "Load with Descendants" context items.
        # "Props" will now also have "Load with Descendants" because something
        # loadable below it is unloaded.
        _validateLoadAndUnloadItems(propsItem, ['Load with Descendants', 'Unload'])
        _validateLoadAndUnloadItems(ball1Item, ['Load', 'Load with Descendants'])
        _validateLoadAndUnloadItems(ball15Item, ['Unload'])

        cmds.undo()

        _validateLoadAndUnloadItems(propsItem, ['Unload'])
        _validateLoadAndUnloadItems(ball1Item, ['Unload'])
        _validateLoadAndUnloadItems(ball15Item, ['Unload'])

        # Unload Props.
        contextOps = ufe.ContextOps.contextOps(propsItem)
        cmd = contextOps.doOpCmd(['Unload'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        # The "Props" prim does not have a payload of its own, so it should
        # only have the "Load with Descendants" item. The Ball assets will also
        # have been unloaded.
        _validateLoadAndUnloadItems(propsItem, ['Load with Descendants'])
        _validateLoadAndUnloadItems(ball1Item, ['Load', 'Load with Descendants'])
        _validateLoadAndUnloadItems(ball15Item, ['Load', 'Load with Descendants'])

        cmds.undo()

        _validateLoadAndUnloadItems(propsItem, ['Unload'])
        _validateLoadAndUnloadItems(ball1Item, ['Unload'])
        _validateLoadAndUnloadItems(ball15Item, ['Unload'])

        cmds.redo()

        _validateLoadAndUnloadItems(propsItem, ['Load with Descendants'])
        _validateLoadAndUnloadItems(ball1Item, ['Load', 'Load with Descendants'])
        _validateLoadAndUnloadItems(ball15Item, ['Load', 'Load with Descendants'])

        # Load Props.
        contextOps = ufe.ContextOps.contextOps(propsItem)
        cmd = contextOps.doOpCmd(['Load with Descendants'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        _validateLoadAndUnloadItems(propsItem, ['Unload'])
        _validateLoadAndUnloadItems(ball1Item, ['Unload'])
        _validateLoadAndUnloadItems(ball15Item, ['Unload'])

        cmds.undo()

        _validateLoadAndUnloadItems(propsItem, ['Load with Descendants'])
        _validateLoadAndUnloadItems(ball1Item, ['Load', 'Load with Descendants'])
        _validateLoadAndUnloadItems(ball15Item, ['Load', 'Load with Descendants'])


    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testAssignExistingMaterialToSingleObject(self):
        """This test assigns an existing material from the stage via ContextOps capabilities."""
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)

        # Create a single object in our stage to test with.
        cmd = contextOps.doOpCmd(['Add New Prim', 'Capsule'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)
        capsuleItem = rootHier.children()[0]
        capsulePrim = usdUtils.getPrimFromSceneItem(capsuleItem)
        contextOps = ufe.ContextOps.contextOps(capsuleItem)

        # We should have no material assigned after creating an object.
        capsulePrim = usdUtils.getPrimFromSceneItem(capsuleItem)
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))

        # Add a new material to the stage (but don't assign it just yet).
        cmd = contextOps.doOpCmd(['Add New Material', 'USD', 'UsdPreviewSurface'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        # We created a new material, but it should not have been assigned to our Capsule.
        capsulePrim = usdUtils.getPrimFromSceneItem(capsuleItem)
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        
        # Now we explictly assign the material to our Capsule.
        cmd = contextOps.doOpCmd(['Assign Existing Material', '', '/Capsule1/UsdPreviewSurface1' ])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        # Confirm the material is assigned.
        capsuleBindAPI = UsdShade.MaterialBindingAPI(capsulePrim)
        self.assertTrue(capsuleBindAPI)
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Capsule1/UsdPreviewSurface1"))

        # Make sure the command plays nice with undo/redo.
        cmds.undo()
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))

        cmds.redo()
        self.assertTrue(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Capsule1/UsdPreviewSurface1"))

        cmds.undo()
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testGeomCoponentAssignment(self):
        '''Duplicate a Maya cube to USD and then assign a material on a face.'''

        cubeXForm, _ = cmds.polyCube(name='MyCube')
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        sl = om.MSelectionList()
        sl.add(psPathStr)
        dagPath = sl.getDagPath(0)
        dagPath.extendToShape()

        with mayaUsd.lib.OpUndoItemList():
            mayaUsd.lib.PrimUpdaterManager.duplicate(
                cmds.ls(cubeXForm, long=True)[0], psPathStr, 
                {'exportComponentTags': True})

        topPath = ufe.PathString.path(psPathStr + ',/' + cubeXForm + "/" + "top")
        topItem = ufe.Hierarchy.createItem(topPath)
        topSubset = UsdGeom.Subset(usdUtils.getPrimFromSceneItem(topItem))

        self.assertEqual(topSubset.GetFamilyNameAttr().Get(), "componentTag")
        self.assertFalse(topSubset.GetPrim().HasAPI(UsdShade.MaterialBindingAPI))

        # We also can check the old sync counters:
        counters= { "resync": cmds.getAttr(psPathStr + '.resyncId'),
                    "update" : cmds.getAttr(psPathStr + '.upid')}

        def assertIsOnlyUpdate(self, counters, shapePathStr):
            resyncCounter = cmds.getAttr(shapePathStr + '.resyncId')
            updateCounter = cmds.getAttr(shapePathStr + '.updateId')
            self.assertEqual(resyncCounter, counters["resync"])
            self.assertGreater(updateCounter, counters["update"])
            counters["resync"] = resyncCounter
            counters["update"] = updateCounter

        def assertIsResync(self, counters, shapePathStr):
            resyncCounter = cmds.getAttr(shapePathStr + '.resyncId')
            updateCounter = cmds.getAttr(shapePathStr + '.updateId')
            self.assertGreater(resyncCounter, counters["resync"])
            self.assertGreater(updateCounter, counters["update"])
            counters["resync"] = resyncCounter
            counters["update"] = updateCounter            

        messageHandler = mayaUtils.TestProxyShapeUpdateHandler(psPathStr)
        messageHandler.snapshot()

        contextOps = ufe.ContextOps.contextOps(topItem)
        cmd = contextOps.doOpCmd(['Assign New Material', 'USD', 'UsdPreviewSurface'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        snIter = iter(ufe.GlobalSelection.get())
        shaderItem = next(snIter)

        self.assertEqual(topSubset.GetFamilyNameAttr().Get(), "materialBind")
        self.assertTrue(topSubset.GetPrim().HasAPI(UsdShade.MaterialBindingAPI))

        # We expect a resync after this assignment:
        self.assertTrue(messageHandler.isResync())
        assertIsResync(self, counters, psPathStr)

        # setting a value the first time is a resync due to the creation of the attribute:
        attrs = ufe.Attributes.attributes(shaderItem)
        metallicAttr = attrs.attribute("inputs:metallic")
        ufeCmd.execute(metallicAttr.setCmd(0.5))
        self.assertTrue(messageHandler.isResync())
        assertIsResync(self, counters, psPathStr)

        # Subsequent changes are updates:
        ufeCmd.execute(metallicAttr.setCmd(0.7))
        self.assertTrue(messageHandler.isUpdate())
        assertIsOnlyUpdate(self, counters, psPathStr)

        # First undo is an update:
        cmds.undo()
        self.assertTrue(messageHandler.isUpdate())
        assertIsOnlyUpdate(self, counters, psPathStr)

        # Second undo is a resync:
        cmds.undo()
        self.assertTrue(messageHandler.isResync())
        assertIsResync(self, counters, psPathStr)

        # Third undo is also resync:
        cmds.undo()
        self.assertTrue(messageHandler.isResync())
        assertIsResync(self, counters, psPathStr)

        # First redo is resync:
        cmds.redo()
        self.assertTrue(messageHandler.isResync())
        assertIsResync(self, counters, psPathStr)

        # Second redo is resync:
        cmds.redo()
        self.assertTrue(messageHandler.isResync())
        assertIsResync(self, counters, psPathStr)

        # Third redo is update:
        cmds.redo()
        self.assertTrue(messageHandler.isUpdate())
        assertIsOnlyUpdate(self, counters, psPathStr)
        currentCacheId = messageHandler.getStageCacheId()

        # Adding custom data to a shader prim is not cause for update. This
        # happens when setting position info on an input node, nodegraph, or
        # material. Also happens when storing alpha channel of a backdrop
        # color.
        shaderHier = ufe.Hierarchy.hierarchy(shaderItem)
        materialItem = shaderHier.parent()
        materialPrim = usdUtils.getPrimFromSceneItem(materialItem)
        materialPrim.SetCustomData({"Autodesk": {"ldx_inputPos" : "-624 -60.5", 
                                                 "ldx_outputPos" : "399 -60"}})
        self.assertTrue(messageHandler.isUnchanged())

        # Changing the whole stage is a resync:
        testFile = testUtils.getTestScene("MaterialX", "MtlxValueTypes.usda")
        cmds.setAttr('{}.filePath'.format(psPathStr), testFile, type='string')

        self.assertTrue(messageHandler.isResync())
        # The old smart signaling for Maya 2024 will not catch that.

        # But that will be the last resync:
        testFile = testUtils.getTestScene("MaterialX", "sin_compound.usda")
        cmds.setAttr('{}.filePath'.format(psPathStr), testFile, type='string')

        self.assertTrue(messageHandler.isUnchanged())

        # Until we pull on the node to get the current stage cache id, which resets
        # the stage listener to the new stage:
        self.assertNotEqual(messageHandler.getStageCacheId(), currentCacheId)

        testFile = testUtils.getTestScene("MaterialX", "MtlxUVStreamTest.usda")
        cmds.setAttr('{}.filePath'.format(psPathStr), testFile, type='string')

        self.assertTrue(messageHandler.isResync())
        # The old smart signaling for Maya 2024 will not catch that.

        messageHandler.terminate()


if __name__ == '__main__':
    unittest.main(verbosity=2)
