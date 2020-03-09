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
        # The top-level context items are visibility (for all prims) and
        # variant sets, for those prims that have them (such as Ball_35).
        contextItems = self.contextOps.getItems([])
        contextItemStrings = [c.item for c in contextItems]

        self.assertIn('Variant Sets', contextItemStrings)
        self.assertIn('Toggle Visibility', contextItemStrings)

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
