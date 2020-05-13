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

import maya.cmds as cmds
import maya.internal.ufeSupport.ufeCmdWrapper as ufeCmd

from pxr import UsdGeom

from ufeTestUtils import usdUtils, mayaUtils
import ufe

import unittest
import os

class ContextOpsTestCase(unittest.TestCase):
    '''Verify the ContextOps interface for the USD runtime.'''

    pluginsLoaded = False
    
    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    def setUp(self):
        ''' Called initially to set up the maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)
        
        # Open top_layer.ma scene in test-samples
        mayaUtils.openTopLayerScene()

        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Clear selection to start off.
        ufe.GlobalSelection.get().clear()
        
        # Select Ball_35.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|world|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        self.ball35Item = ufe.Hierarchy.createItem(ball35Path)

        ufe.GlobalSelection.get().append(self.ball35Item)

        # Create a ContextOps interface for it.
        self.contextOps = ufe.ContextOps.contextOps(self.ball35Item)

    def testGetItems(self):
        # The top-level context items are:
        # - visibility (for all prims with visibility attribute)
        # - variant sets, for those prims that have them (such as Ball_35)
        # - add new prim (for all prims)
        contextItems = self.contextOps.getItems([])
        contextItemStrings = [c.item for c in contextItems]

        attrs = ufe.Attributes.attributes(self.contextOps.sceneItem())
        self.assertIsNotNone(attrs)
        hasVisibility = attrs.hasAttribute(UsdGeom.Tokens.visibility)

        self.assertIn('Variant Sets', contextItemStrings)
        if hasVisibility:
            self.assertIn('Toggle Visibility', contextItemStrings)
        else:
            self.assertNotIn('Toggle Visibility', contextItemStrings)
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

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2015', 'testAddNewPrim only available in UFE preview version 0.2.15 and greater')
    def testAddNewPrim(self):
        # Create a ContextOps interface for the proxy shape.
        proxyShapePath = ufe.Path([mayaUtils.createUfePathSegment("|world|stage|stageShape")])
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

        # Create a ContextOps interface for it.
        contextOps = ufe.ContextOps.contextOps(xformItem)

        # Add a new prim.
        cmd = contextOps.doOpCmd(['Add New Prim', 'Xform'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        # The xform prim should now have a single UFE child item.
        xformHier = ufe.Hierarchy.hierarchy(xformItem)
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 1)

        # Add another prim
        cmd = contextOps.doOpCmd(['Add New Prim', 'Capsule'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        # The xform prim should now have two UFE child items.
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 2)

        # Undo will remove the new prim, meaning one less child.
        cmds.undo()
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 1)

        # Undo again will remove the first added prim, meaning no children.
        cmds.undo()
        self.assertFalse(xformHier.hasChildren())

        cmds.redo()
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 1)

        cmds.redo()
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 2)
