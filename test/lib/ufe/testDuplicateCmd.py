#!/usr/bin/env python

#
# Copyright 2019 Autodesk
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

import usdUtils, mayaUtils
import ufe

import unittest

class DuplicateCmdTestCase(unittest.TestCase):
    '''Verify the Maya delete command, for multiple runtimes.

    UFE Feature : SceneItemOps
    Maya Feature : duplicate
    Action : Duplicate objects in the scene.
    Applied On Selection :
        - Multiple Selection [Mixed, Non-Maya].  Maya-only selection tested by
          Maya.
    Undo/Redo Test : Yes
    Expect Results To Test :
        - Duplicate objects in the scene.
    Edge Cases :
        - None.
    '''

    pluginsLoaded = False
    
    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    def setUp(self):
        ''' Called initially to set up the Maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)
        
        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()
        
        # Create some extra Maya nodes
        cmds.polySphere()

        # Clear selection to start off
        cmds.select(clear=True)

    def testDuplicate(self):
        '''Duplicate Maya and USD objects.'''

        # Select two objects, one Maya, one USD.
        spherePath = ufe.Path(mayaUtils.createUfePathSegment("|pSphere1"))
        sphereItem = ufe.Hierarchy.createItem(spherePath)
        sphereHierarchy = ufe.Hierarchy.hierarchy(sphereItem)
        worldItem = sphereHierarchy.parent()
        
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment(
                "|world|transform1|proxyShape1"),
             usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)
        ball35Hierarchy = ufe.Hierarchy.hierarchy(ball35Item)
        propsItem = ball35Hierarchy.parent()

        worldHierarchy = ufe.Hierarchy.hierarchy(worldItem)
        worldChildrenPre = worldHierarchy.children()
        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildrenPre = propsHierarchy.children()

        ufe.GlobalSelection.get().append(sphereItem)
        ufe.GlobalSelection.get().append(ball35Item)

        # Set the edit target to the layer in which Ball_35 is defined (has a
        # primSpec, in USD terminology).  Otherwise, duplication will not find
        # a source primSpec to copy.  Layers are the (anonymous) session layer,
        # the root layer, then the Assembly_room_set sublayer.  Trying to find
        # the layer by name is not practical, as it requires the full path
        # name, which potentially differs per run-time environment.
        ball35Prim = usdUtils.getPrimFromSceneItem(ball35Item)
        stage = ball35Prim.GetStage()

        layer = stage.GetLayerStack()[2]
        stage.SetEditTarget(layer)

        cmds.duplicate()

        # The duplicate command doesn't return duplicated non-Maya UFE objects.
        # They are in the selection, in the same order as the sources.
        snIter = iter(ufe.GlobalSelection.get())
        sphereDupItem = next(snIter)
        sphereDupName = str(sphereDupItem.path().back())
        ball35DupItem = next(snIter)
        ball35DupName = str(ball35DupItem.path().back())

        worldChildren = worldHierarchy.children()
        propsChildren = propsHierarchy.children()

        self.assertEqual(len(worldChildren)-len(worldChildrenPre), 1)
        self.assertEqual(len(propsChildren)-len(propsChildrenPre), 1)

        self.assertIn(sphereDupItem, worldChildren)
        self.assertIn(ball35DupItem, propsChildren)

        cmds.undo()

        # The duplicated items should no longer appear in the child list of
        # their parents.

        def childrenNames(children):
            return [str(child.path().back()) for child in children]

        worldHierarchy = ufe.Hierarchy.hierarchy(worldItem)
        worldChildren = worldHierarchy.children()
        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildren = propsHierarchy.children()

        worldChildrenNames = childrenNames(worldChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertNotIn(sphereDupName, worldChildrenNames)
        self.assertNotIn(ball35DupName, propsChildrenNames)

        # MAYA-92264: because of USD bug, redo doesn't work.
        """
        return
        cmds.redo()

        snIter = iter(ufe.GlobalSelection.get())
        sphereDupItem = next(snIter)
        ball35DupItem = next(snIter)

        worldChildren = worldHierarchy.children()
        propsChildren = propsHierarchy.children()

        self.assertEqual(len(worldChildren)-len(worldChildrenPre), 1)
        self.assertEqual(len(propsChildren)-len(propsChildrenPre), 1)

        self.assertIn(sphereDupItem, worldChildren)
        self.assertIn(ball35DupItem, propsChildren)
        """

        # The duplicated items should not be assigned to the name of a
        # deactivated USD item.

        cmds.select(clear=True)

        # Delete the even numbered props:
        evenPropsChildrenPre = propsChildrenPre[0:35:2]
        for propChild in evenPropsChildrenPre:
            ufe.GlobalSelection.get().append(propChild)
        cmds.delete()

        worldHierarchy = ufe.Hierarchy.hierarchy(worldItem)
        worldChildren = worldHierarchy.children()
        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildren = propsHierarchy.children()
        propsChildrenPostDel = propsHierarchy.children()

        # Duplicate Ball_1
        ufe.GlobalSelection.get().append(propsChildrenPostDel[0])

        cmds.duplicate()

        snIter = iter(ufe.GlobalSelection.get())
        ballDupItem = next(snIter)
        ballDupName = str(ballDupItem.path().back())

        self.assertNotIn(ballDupItem, propsChildrenPostDel)
        self.assertNotIn(ballDupName, propsChildrenNames)
        self.assertEqual(ballDupName, "Ball_36")

        cmds.undo() # undo duplication
        cmds.undo() # undo deletion
