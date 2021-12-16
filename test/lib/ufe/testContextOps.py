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

from pxr import UsdGeom

from maya import cmds
from maya import standalone
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

import ufe

import os
import unittest


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

    def testAddNewPrim(self):
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
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

    def testAddNewPrimWithDelete(self):
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
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
        # instead makes it inactive).
        cmds.pickWalk(d='down')
        cmds.delete()

        # The proxy shape should now have no UFE child items (since we skip inactive).
        self.assertFalse(proxyShapehier.hasChildren())
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


if __name__ == '__main__':
    unittest.main(verbosity=2)
