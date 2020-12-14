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

import ufeUtils, usdUtils, mayaUtils
import ufe

import unittest
import os

def childrenNames(children):
    return [str(child.path().back()) for child in children]

class TestObserver(ufe.Observer):
    def __init__(self):
        super(TestObserver, self).__init__()
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

class DeleteCmdTestCase(unittest.TestCase):
    '''Verify the Maya delete command, for multiple runtimes.

    UFE Feature : SceneItemOps
    Maya Feature : delete
    Action : Remove object from scene.
    Applied On Selection / Command Arguments:
        - Multiple Selection [Mixed, Non-Maya].  Maya-only selection tested by
          Maya.
    Undo/Redo Test : Yes
    Expect Results To Test :
        - Delete removes object from scene.
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

    def testDelete(self):
        '''Delete Maya and USD objects.'''

        # Create our UFE notification observer
        ufeObs = TestObserver()

        if(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2021'):
            ufe.Scene.addObjectDeleteObserver(ufeObs)
            ufe.Scene.addObjectAddObserver(ufeObs)
        else:
            ufe.Scene.addObserver(ufeObs)

        # Select two objects, one Maya, one USD.
        spherePath = ufe.Path(mayaUtils.createUfePathSegment("|pSphere1"))
        sphereItem = ufe.Hierarchy.createItem(spherePath)
        sphereShapePath = ufe.Path(
            mayaUtils.createUfePathSegment("|pSphere1|pSphereShape1"))
        sphereShapeItem = ufe.Hierarchy.createItem(sphereShapePath)

        mayaSegment = mayaUtils.createUfePathSegment(
            "|transform1|proxyShape1")
        ball35Path = ufe.Path(
            [mayaSegment,
             usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)
        propsPath = ufe.Path(
            [mayaSegment, usdUtils.createUfePathSegment("/Room_set/Props")])
        propsItem = ufe.Hierarchy.createItem(propsPath)

        sphereShapeName = str(sphereShapeItem.path().back())
        ball35Name = str(ball35Item.path().back())

        ufe.GlobalSelection.get().append(sphereShapeItem)
        ufe.GlobalSelection.get().append(ball35Item)

        # Before delete, each item is a child of its parent.
        sphereHierarchy = ufe.Hierarchy.hierarchy(sphereItem)
        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)

        sphereChildren = sphereHierarchy.children()
        propsChildren = propsHierarchy.children()

        sphereChildrenNames = childrenNames(sphereChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertIn(sphereShapeItem, sphereChildren)
        self.assertIn(ball35Item, propsChildren)
        self.assertIn(sphereShapeName, sphereChildrenNames)
        self.assertIn(ball35Name, propsChildrenNames)

        ufeObs.reset()
        cmds.delete()

        # We deleted two items.
        self.assertEqual(ufeObs.nbDeleteNotif(), 2)
        self.assertEqual(ufeObs.nbAddNotif(), 0)

        sphereChildren = sphereHierarchy.children()
        propsChildren = propsHierarchy.children()

        sphereChildrenNames = childrenNames(sphereChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertNotIn(sphereShapeName, sphereChildrenNames)
        self.assertNotIn(ball35Name, propsChildrenNames)

        cmds.undo()

        # After the undo we added two items back.
        self.assertEqual(ufeObs.nbDeleteNotif(), 2)
        self.assertEqual(ufeObs.nbAddNotif(), 2)

        sphereChildren = sphereHierarchy.children()
        propsChildren = propsHierarchy.children()

        sphereChildrenNames = childrenNames(sphereChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertIn(sphereShapeItem, sphereChildren)
        self.assertIn(ball35Item, propsChildren)
        self.assertIn(sphereShapeName, sphereChildrenNames)
        self.assertIn(ball35Name, propsChildrenNames)

        cmds.redo()

        # After the redo we again deleted two items.
        self.assertEqual(ufeObs.nbDeleteNotif(), 4)
        self.assertEqual(ufeObs.nbAddNotif(), 2)

        sphereChildren = sphereHierarchy.children()
        propsChildren = propsHierarchy.children()

        sphereChildrenNames = childrenNames(sphereChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertNotIn(sphereShapeName, sphereChildrenNames)
        self.assertNotIn(ball35Name, propsChildrenNames)

        # undo to restore state to original.
        cmds.undo()

        # After the undo we again added two items back.
        self.assertEqual(ufeObs.nbDeleteNotif(), 4)
        self.assertEqual(ufeObs.nbAddNotif(), 4)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testDeleteArgs only available in UFE v2 or greater.')
    def testDeleteArgs(self):
        '''Delete Maya and USD objects passed as command arguments.'''

        # Create our UFE notification observer
        ufeObs = TestObserver()
        if(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2021'):
            ufe.Scene.addObjectDeleteObserver(ufeObs)
            ufe.Scene.addObjectAddObserver(ufeObs)
        else:
            ufe.Scene.addObserver(ufeObs)

        spherePath = ufe.Path(mayaUtils.createUfePathSegment("|pSphere1"))
        sphereItem = ufe.Hierarchy.createItem(spherePath)
        sphereShapePath = ufe.Path(
            mayaUtils.createUfePathSegment("|pSphere1|pSphereShape1"))
        sphereShapeItem = ufe.Hierarchy.createItem(sphereShapePath)

        mayaSegment = mayaUtils.createUfePathSegment(
            "|transform1|proxyShape1")
        ball35Path = ufe.Path(
            [mayaSegment,
             usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)
        ball34Path = ufe.Path(
            [mayaSegment,
             usdUtils.createUfePathSegment("/Room_set/Props/Ball_34")])
        ball34Item = ufe.Hierarchy.createItem(ball34Path)
        propsPath = ufe.Path(
            [mayaSegment, usdUtils.createUfePathSegment("/Room_set/Props")])
        propsItem = ufe.Hierarchy.createItem(propsPath)

        sphereShapeName = str(sphereShapeItem.path().back())
        ball35Name = str(ball35Item.path().back())
        ball34Name = str(ball34Item.path().back())

        # Before delete, each item is a child of its parent.
        sphereHierarchy = ufe.Hierarchy.hierarchy(sphereItem)
        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)

        sphereChildren = sphereHierarchy.children()
        propsChildren = propsHierarchy.children()

        sphereChildrenNames = childrenNames(sphereChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertIn(sphereShapeItem, sphereChildren)
        self.assertIn(ball35Item, propsChildren)
        self.assertIn(sphereShapeName, sphereChildrenNames)
        self.assertIn(ball35Name, propsChildrenNames)

        ball34PathString = ufe.PathString.string(ball34Path)
        self.assertEqual(
            ball34PathString,
            "|transform1|proxyShape1,/Room_set/Props/Ball_34")

        # Test that "|world" prefix is optional for multi-segment paths.
        ball35PathString = "|transform1|proxyShape1,/Room_set/Props/Ball_35"

        ufeObs.reset()
        cmds.delete(
            ball35PathString, ball34PathString, "|pSphere1|pSphereShape1")

        # We deleted 3 items.
        self.assertEqual(ufeObs.nbDeleteNotif(), 3)
        self.assertEqual(ufeObs.nbAddNotif(), 0)

        sphereChildren = sphereHierarchy.children()
        propsChildren = propsHierarchy.children()

        sphereChildrenNames = childrenNames(sphereChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertNotIn(sphereShapeName, sphereChildrenNames)
        self.assertNotIn(ball35Name, propsChildrenNames)
        self.assertNotIn(ball34Name, propsChildrenNames)
        self.assertFalse(cmds.objExists("|pSphere1|pSphereShape1"))

        cmds.undo()

        # After the undo we added three items back.
        self.assertEqual(ufeObs.nbDeleteNotif(), 3)
        self.assertEqual(ufeObs.nbAddNotif(), 3)

        sphereChildren = sphereHierarchy.children()
        propsChildren = propsHierarchy.children()

        sphereChildrenNames = childrenNames(sphereChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertIn(sphereShapeItem, sphereChildren)
        self.assertIn(ball35Item, propsChildren)
        self.assertIn(ball34Item, propsChildren)
        self.assertIn(sphereShapeName, sphereChildrenNames)
        self.assertIn(ball35Name, propsChildrenNames)
        self.assertIn(ball34Name, propsChildrenNames)
        self.assertTrue(cmds.objExists("|pSphere1|pSphereShape1"))

        cmds.redo()

        # After the redo we again deleted three items.
        self.assertEqual(ufeObs.nbDeleteNotif(), 6)
        self.assertEqual(ufeObs.nbAddNotif(), 3)

        sphereChildren = sphereHierarchy.children()
        propsChildren = propsHierarchy.children()

        sphereChildrenNames = childrenNames(sphereChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertNotIn(sphereShapeName, sphereChildrenNames)
        self.assertNotIn(ball35Name, propsChildrenNames)
        self.assertNotIn(ball34Name, propsChildrenNames)
        self.assertFalse(cmds.objExists("|pSphere1|pSphereShape1"))
