#!/usr/bin/env python

#
# Copyright 2021 Autodesk
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
import ufeUtils
import usdUtils

import mayaUsd.ufe

from maya import cmds
from maya import standalone

import ufe

import os
import unittest


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
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

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

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() <= 1, 'only available in UFE v1 or less.')
    def testDeleteUfe1(self):
        '''Delete Maya and USD objects.'''

        # Create our UFE notification observer
        ufeObs = TestObserver()

        if ufeUtils.ufeFeatureSetVersion() < 2:
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

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'only available in UFE v2 or more.')
    def testDelete(self):
        '''Delete Maya and USD objects.'''

        # Create our UFE notification observer
        ufeObs = TestObserver()

        if ufeUtils.ufeFeatureSetVersion() < 2:
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

        # We only deleted the maya object.
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        self.assertEqual(ufeObs.nbAddNotif(), 0)

        sphereChildren = sphereHierarchy.children()
        propsChildren = propsHierarchy.children()

        sphereChildrenNames = childrenNames(sphereChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertNotIn(sphereShapeName, sphereChildrenNames)
        # ball_35 cannot be deleted check its still in list
        self.assertIn(ball35Name, propsChildrenNames)

        cmds.undo()

        # After the undo we added one items back.
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        self.assertEqual(ufeObs.nbAddNotif(), 1)

        sphereChildren = sphereHierarchy.children()
        propsChildren = propsHierarchy.children()

        sphereChildrenNames = childrenNames(sphereChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertIn(sphereShapeItem, sphereChildren)
        self.assertIn(ball35Item, propsChildren)
        self.assertIn(sphereShapeName, sphereChildrenNames)
        self.assertIn(ball35Name, propsChildrenNames)
        
        cmds.redo()

        # After the redo we again deleted one item.
        self.assertEqual(ufeObs.nbDeleteNotif(), 2)
        self.assertEqual(ufeObs.nbAddNotif(), 1)

        sphereChildren = sphereHierarchy.children()
        propsChildren = propsHierarchy.children()

        sphereChildrenNames = childrenNames(sphereChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertNotIn(sphereShapeName, sphereChildrenNames)
        self.assertIn(ball35Name, propsChildrenNames)

        # undo to restore state to original.
        cmds.undo()

        # After the undo we again added the sphere item back.
        self.assertEqual(ufeObs.nbDeleteNotif(), 2)
        self.assertEqual(ufeObs.nbAddNotif(), 2)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testDeleteArgs only available in UFE v2 or greater.')
    def testDeleteArgs(self):
        '''Delete Maya and USD objects passed as command arguments.'''

        # Create our UFE notification observer
        ufeObs = TestObserver()
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
        
        # We deleted 1 items.
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        self.assertEqual(ufeObs.nbAddNotif(), 0)

        sphereChildren = sphereHierarchy.children()
        propsChildren = propsHierarchy.children()

        sphereChildrenNames = childrenNames(sphereChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertNotIn(sphereShapeName, sphereChildrenNames)
        self.assertIn(ball35Name, propsChildrenNames)
        self.assertIn(ball34Name, propsChildrenNames)
        self.assertFalse(cmds.objExists("|pSphere1|pSphereShape1"))

        cmds.undo()
        
        # After the undo we added one item back.
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        self.assertEqual(ufeObs.nbAddNotif(), 1)

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
        
        # After the redo we again deleted one item.
        self.assertEqual(ufeObs.nbDeleteNotif(), 2)
        self.assertEqual(ufeObs.nbAddNotif(), 1)

        sphereChildren = sphereHierarchy.children()
        propsChildren = propsHierarchy.children()

        sphereChildrenNames = childrenNames(sphereChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertNotIn(sphereShapeName, sphereChildrenNames)
        self.assertIn(ball35Name, propsChildrenNames)
        self.assertIn(ball34Name, propsChildrenNames)
        self.assertFalse(cmds.objExists("|pSphere1|pSphereShape1"))
        
        cmds.undo()

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testDeleteProxyShape only available in UFE v2 or greater.') 
    def testDeleteProxyShape(self):
        '''Delete proxy shape.'''

        # Create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        mayaSegment = mayaUtils.createUfePathSegment(
            "|transform1|proxyShape1")
        usdSegment = usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")

        ball35Path = ufe.Path([mayaSegment, usdSegment])
        ball35PathStr = ','.join(
            [str(segment) for segment in ball35Path.segments])

        self.assertEqual(len(cmds.ls('proxyShape1')), 1)

        # delete the proxy shape node itself. 
        cmds.delete('|transform1|proxyShape1')

        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        
        #restore
        cmds.undo()

        ufeObs.reset()
        self.assertEqual(len(cmds.ls('proxyShape1')), 1)

        #cannot be deleted as it has a variant arc
        cmds.delete(ball35PathStr)
        self.assertEqual(ufeObs.nbDeleteNotif(), 0)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testDeleteProxyShape only available in UFE v2 or greater.') 
    def testDeleteRestrictionSameLayerDef(self):
        '''Restrict deleting USD node. Cannot delete a prim defined on another layer.'''
        
        # Create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|transform1|proxyShape1')
        usdPathSegment = usdUtils.createUfePathSegment('/Room_set/Props/Ball_35')
        ball35Path = ufe.Path([mayaPathSegment, usdPathSegment])
        ball35PathStr = ','.join(
            [str(segment) for segment in ball35Path.segments])
        
        ball35Item = ufe.Hierarchy.createItem(ball35Path)
        
        ufe.GlobalSelection.get().append(ball35Item)
        
        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
        
        # check GetLayerStack behavior
        self.assertEqual(stage.GetLayerStack()[0], stage.GetSessionLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        cmds.delete()    
        
        self.assertEqual(ufeObs.nbDeleteNotif(), 0)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testDeleteProxyShape only available in UFE v2 or greater.') 
    def testDeleteRestrictionHasSpecs(self):
        '''Restrict deleting USD node. Cannot delete a node that doesn't contribute to the final composed prim'''

        cmds.file(new=True, force=True)

        # open appleBite.ma scene in testSamples
        mayaUtils.openAppleBiteScene()

        # Create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # clear selection to start off
        cmds.select(clear=True)
        
        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Asset_flattened_instancing_and_class_removed_usd|Asset_flattened_instancing_and_class_removed_usdShape')
        usdPathSegment = usdUtils.createUfePathSegment('/apple/payload/geo')
        geoPath = ufe.Path([mayaPathSegment, usdPathSegment])
        geoItem = ufe.Hierarchy.createItem(geoPath)
        globalSn = ufe.GlobalSelection.get()        
        globalSn.append(geoItem)
        self.assertEqual(len(ufe.GlobalSelection.get()), 1)

        cmds.delete()
        # not deleted
        self.assertEqual(ufeObs.nbDeleteNotif(), 0)

        # Clear the selection.

        globalSn.clear()
        self.assertTrue(globalSn.empty())

        usdPathSegment = usdUtils.createUfePathSegment('/apple/payload')
        geoPath = ufe.Path([mayaPathSegment, usdPathSegment])
        geoItem = ufe.Hierarchy.createItem(geoPath)

        globalSn.append(geoItem)
        self.assertEqual(len(ufe.GlobalSelection.get()), 1)

        cmds.delete()
        #deleted
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testDeleteProxyShape only available in UFE v2 or greater.') 
    def testDeleteUniqueName(self):

        cmds.file(new=True, force=True)

        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()
		
        # Create some extra Maya nodes
        cmds.polySphere()
		
        # clear selection to start off
        cmds.select(clear=True)

        # Create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)
		
        # select a maya object.
        spherePath = ufe.Path(mayaUtils.createUfePathSegment("|pSphere1"))
        sphereItem = ufe.Hierarchy.createItem(spherePath)
        sphereShapePath = ufe.Path(
            mayaUtils.createUfePathSegment("|pSphere1|pSphereShape1"))
        sphereShapeItem = ufe.Hierarchy.createItem(sphereShapePath)

        ufe.GlobalSelection.get().append(sphereShapeItem)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        usdPathSegment = usdUtils.createUfePathSegment('/TreeBase/trunk')
        trunkPath = ufe.Path([mayaPathSegment, usdPathSegment])
        trunkItem = ufe.Hierarchy.createItem(trunkPath)

        ufe.GlobalSelection.get().append(trunkItem)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # by default edit target is set to the Rootlayer.
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        # delete selected item `/TreeBase/trunk` `
        cmds.delete()
        # deleted two items
        self.assertEqual(ufeObs.nbDeleteNotif() , 2)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testDeleteProxyShape only available in UFE v2 or greater.') 
    def testDeleteRestrictionVariant(self):
        '''Deleting prims inside a variantSet is not allowed.'''

        cmds.file(new=True, force=True)

        # open Variant.ma scene in testSamples
        mayaUtils.openVariantSetScene()

        # Create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # stage
        mayaPathSegment = mayaUtils.createUfePathSegment('|Variant_usd|Variant_usdShape')
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # first check that we have a VariantSets
        objectPrim = stage.GetPrimAtPath('/objects')
        self.assertTrue(objectPrim.HasVariantSets())

        # Geom
        usdPathSegment = usdUtils.createUfePathSegment('/objects/Geom')
        geomPath = ufe.Path([mayaPathSegment, usdPathSegment])
        geomItem = ufe.Hierarchy.createItem(geomPath)
        geomPrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(geomPath))

        # Small_Potato
        smallPotatoUsdPathSegment = usdUtils.createUfePathSegment('/objects/Geom/Small_Potato')
        smallPotatoPath = ufe.Path([mayaPathSegment, smallPotatoUsdPathSegment])
        smallPotatoItem = ufe.Hierarchy.createItem(smallPotatoPath)
        smallPotatoPrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(smallPotatoPath))

        # add Geom to selection list
        ufe.GlobalSelection.get().append(geomItem)

        # get prim spec for Geom prim
        primspecGeom = stage.GetEditTarget().GetPrimSpecForScenePath(geomPrim.GetPath());

        # primSpec is expected to be None
        self.assertIsNone(primspecGeom)

        cmds.delete()
        # expect no delete
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)

        # clear selection
        cmds.select(clear=True)

        # add Small_Potato to selection list 
        ufe.GlobalSelection.get().append(smallPotatoItem)

        # get prim spec for Small_Potato prim
        primspecSmallPotato = stage.GetEditTarget().GetPrimSpecForScenePath(smallPotatoPrim.GetPath())

        # primSpec is expected to be None
        self.assertIsNone(primspecSmallPotato)

        # delete "/objects/Geom/Small_Potato"
        # expect no delete
        cmds.delete()
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)

        # clear selection
        cmds.select(clear=True)

        usdPathSegment = usdUtils.createUfePathSegment('/objects')
        geomPath = ufe.Path([mayaPathSegment, usdPathSegment])
        geomItem = ufe.Hierarchy.createItem(geomPath)
        # select objects
        ufe.GlobalSelection.get().append(geomItem)
        cmds.delete()
        #success
        self.assertEqual(ufeObs.nbDeleteNotif() , 1)

if __name__ == '__main__':
    unittest.main(verbosity=2)
