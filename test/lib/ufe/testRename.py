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

from ufeTestUtils import usdUtils, mayaUtils
import ufe
import mayaUsd.ufe

import unittest
import re

class TestObserver(ufe.Observer):
    def __init__(self):
        super(TestObserver, self).__init__()
        self.unexpectedNotif = 0
        self.renameNotif = 0

    def __call__(self, notification):
        if isinstance(notification, (ufe.ObjectAdd, ufe.ObjectDelete)):
            self.unexpectedNotif += 1
        if isinstance(notification, ufe.ObjectRename):
            self.renameNotif += 1

    def notifications(self):
        return self.renameNotif

    def receivedUnexpectedNotif(self):
        return self.unexpectedNotif > 0

class RenameTestCase(unittest.TestCase):
    '''Test renaming a UFE scene item and its ancestors.

    Renaming should not affect UFE lookup, including renaming the proxy shape.
    '''

    pluginsLoaded = False
    
    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    @classmethod
    def tearDownClass(cls):
        cmds.file(new=True, force=True)

    def setUp(self):
        ''' Called initially to set up the Maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)
        
        # Open top_layer.ma scene in test-samples
        mayaUtils.openTopLayerScene()
        
        # Clear selection to start off
        cmds.select(clear=True)

    def testRenameProxyShape(self):
        '''Rename proxy shape, UFE lookup should succeed.'''

        mayaSegment = mayaUtils.createUfePathSegment(
            "|world|transform1|proxyShape1")
        usdSegment = usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")

        ball35Path = ufe.Path([mayaSegment, usdSegment])
        ball35PathStr = ','.join(
            [str(segment) for segment in ball35Path.segments])

        # Because of difference in Python binding systems (pybind11 for UFE,
        # Boost Python for mayaUsd and USD), need to pass in strings to
        # mayaUsd functions.  Multi-segment UFE paths need to have
        # comma-separated segments.
        def assertStageAndPrimAccess(
                proxyShapeSegment, primUfePathStr, primSegment):
            proxyShapePathStr = str(proxyShapeSegment)

            stage     = mayaUsd.ufe.getStage(proxyShapePathStr)
            prim      = mayaUsd.ufe.ufePathToPrim(primUfePathStr)
            stagePath = mayaUsd.ufe.stagePath(stage)

            self.assertIsNotNone(stage)
            self.assertEqual(stagePath, proxyShapePathStr)
            self.assertTrue(prim.IsValid())
            self.assertEqual(str(prim.GetPath()), str(primSegment))

        assertStageAndPrimAccess(mayaSegment, ball35PathStr, usdSegment)

        # Rename the proxy shape node itself.  Stage and prim access should
        # still be valid, with the new path.
        mayaSegment = mayaUtils.createUfePathSegment(
            "|world|transform1|potato")
        cmds.rename('|transform1|proxyShape1', 'potato')
        self.assertEqual(len(cmds.ls('potato')), 1)

        ball35Path = ufe.Path([mayaSegment, usdSegment])
        ball35PathStr = ','.join(
            [str(segment) for segment in ball35Path.segments])

        assertStageAndPrimAccess(mayaSegment, ball35PathStr, usdSegment)

    def testRename(self):
        # open tree.ma scene in test-samples
        mayaUtils.openTreeScene()

        # clear selection to start off
        cmds.select(clear=True)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|world|Tree_usd|Tree_usdShape')
        usdPathSegment = usdUtils.createUfePathSegment('/TreeBase')
        treebasePath = ufe.Path([mayaPathSegment, usdPathSegment])
        treebaseItem = ufe.Hierarchy.createItem(treebasePath)

        ufe.GlobalSelection.get().append(treebaseItem)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # by default edit target is set to the Rootlayer.
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        self.assertTrue(stage.GetRootLayer().GetPrimAtPath("/TreeBase"))

        # get default prim
        defaultPrim = stage.GetDefaultPrim()
        self.assertEqual(defaultPrim.GetName(), 'TreeBase')

        # TreeBase has two childern: leavesXform, trunk
        assert len(defaultPrim.GetChildren()) == 2

        # get prim spec for defaultPrim
        primspec = stage.GetEditTarget().GetPrimSpecForScenePath(defaultPrim.GetPath());

        # set primspec name
        primspec.name = "TreeBase_potato"

        # get the renamed prim
        renamedPrim = stage.GetPrimAtPath('/TreeBase_potato')

        # One must use the SdfLayer API for setting the defaultPrim when you rename the prim it identifies.
        stage.SetDefaultPrim(renamedPrim);

        # get defaultPrim again
        defaultPrim = stage.GetDefaultPrim()
        self.assertEqual(defaultPrim.GetName(), 'TreeBase_potato')

        # make sure we have a valid prims after the primspec rename 
        assert stage.GetPrimAtPath('/TreeBase_potato')
        assert stage.GetPrimAtPath('/TreeBase_potato/leavesXform')

        # prim should be called TreeBase_potato
        potatoPrim = stage.GetPrimAtPath('/TreeBase_potato')
        self.assertEqual(potatoPrim.GetName(), 'TreeBase_potato')

        # prim should be called leaves 
        leavesPrimSpec = stage.GetObjectAtPath('/TreeBase_potato/leavesXform/leaves')
        self.assertEqual(leavesPrimSpec.GetName(), 'leaves')

    def testRenameUndo(self):
        '''Rename USD node.'''

        # open usdCylinder.ma scene in test-samples
        mayaUtils.openCylinderScene()

        # clear selection to start off
        cmds.select(clear=True)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|world|mayaUsdTransform|shape')
        usdPathSegment = usdUtils.createUfePathSegment('/pCylinder1')
        cylinderPath = ufe.Path([mayaPathSegment, usdPathSegment])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)
        cylinderHierarchy = ufe.Hierarchy.hierarchy(cylinderItem)
        propsItem = cylinderHierarchy.parent()

        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildrenPre = propsHierarchy.children()

        ufe.GlobalSelection.get().append(cylinderItem)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # check GetLayerStack behavior
        self.assertEqual(stage.GetLayerStack()[0], stage.GetSessionLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        # by default edit target is set to the Rootlayer.
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        # rename
        cylinderItemType = cylinderItem.nodeType()
        newName = 'pCylinder1_Renamed'
        cmds.rename(newName)

        # The renamed item is in the selection.
        snIter = iter(ufe.GlobalSelection.get())
        pCylinder1Item = next(snIter)
        pCylinder1RenName = str(pCylinder1Item.path().back())

        self.assertEqual(pCylinder1RenName, newName)

        propsChildren = propsHierarchy.children()

        self.assertEqual(len(propsChildren), len(propsChildrenPre))
        self.assertIn(pCylinder1Item, propsChildren)

        cmds.undo()
        self.assertEqual(cylinderItemType, ufe.GlobalSelection.get().back().nodeType())

        def childrenNames(children):
           return [str(child.path().back()) for child in children]

        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildren = propsHierarchy.children()
        propsChildrenNames = childrenNames(propsChildren)

        self.assertNotIn(pCylinder1RenName, propsChildrenNames)
        self.assertIn('pCylinder1', propsChildrenNames)
        self.assertEqual(len(propsChildren), len(propsChildrenPre))

        cmds.redo()
        self.assertEqual(cylinderItemType, ufe.GlobalSelection.get().back().nodeType())

        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildren = propsHierarchy.children()
        propsChildrenNames = childrenNames(propsChildren)

        self.assertIn(pCylinder1RenName, propsChildrenNames)
        self.assertNotIn('pCylinder1', propsChildrenNames)
        self.assertEqual(len(propsChildren), len(propsChildrenPre))

    def testRenameRestrictionSameLayerDef(self):
        '''Restrict renaming USD node. Cannot rename a prim defined on another layer.'''

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|world|transform1|proxyShape1')
        usdPathSegment = usdUtils.createUfePathSegment('/Room_set/Props/Ball_35')
        ball35Path = ufe.Path([mayaPathSegment, usdPathSegment])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)

        ufe.GlobalSelection.get().append(ball35Item)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # check GetLayerStack behavior
        self.assertEqual(stage.GetLayerStack()[0], stage.GetSessionLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        # expect the exception happens
        with self.assertRaises(RuntimeError):
            newName = 'Ball_35_Renamed'
            cmds.rename(newName)

    def testRenameRestrictionOtherLayerOpinions(self):
        '''Restrict renaming USD node. Cannot rename a prim with definitions or opinions on other layers.'''

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|world|transform1|proxyShape1')
        usdPathSegment = usdUtils.createUfePathSegment('/Room_set/Props/Ball_35')
        ball35Path = ufe.Path([mayaPathSegment, usdPathSegment])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)

        ufe.GlobalSelection.get().append(ball35Item)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # set the edit target to Assembly_room_set.usda
        stage.SetEditTarget(stage.GetLayerStack()[2])
        self.assertEqual(stage.GetEditTarget().GetLayer().GetDisplayName(), "Assembly_room_set.usda")

        # expect the exception happens
        with self.assertRaises(RuntimeError):
           newName = 'Ball_35_Renamed'
           cmds.rename(newName)

    def testRenameRestrictionHasSpecs(self):
        '''Restrict renaming USD node. Cannot rename a node that doesn't contribute to the final composed prim'''

        # open appleBite.ma scene in test-samples
        mayaUtils.openAppleBiteScene()

        # clear selection to start off
        cmds.select(clear=True)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|world|Asset_flattened_instancing_and_class_removed_usd|Asset_flattened_instancing_and_class_removed_usdShape')
        usdPathSegment = usdUtils.createUfePathSegment('/apple/payload/geo')
        geoPath = ufe.Path([mayaPathSegment, usdPathSegment])
        geoItem = ufe.Hierarchy.createItem(geoPath)

        ufe.GlobalSelection.get().append(geoItem)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # rename "/apple/payload/geo" to "/apple/payload/geo_renamed"
        # expect the exception happens
        with self.assertRaises(RuntimeError):
            cmds.rename("geo_renamed")

    def testRenameUniqueName(self):
        # open tree.ma scene in test-samples
        mayaUtils.openTreeScene()

        # clear selection to start off
        cmds.select(clear=True)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|world|Tree_usd|Tree_usdShape')
        usdPathSegment = usdUtils.createUfePathSegment('/TreeBase/trunk')
        trunkPath = ufe.Path([mayaPathSegment, usdPathSegment])
        trunkItem = ufe.Hierarchy.createItem(trunkPath)

        ufe.GlobalSelection.get().append(trunkItem)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # by default edit target is set to the Rootlayer.
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        # rename `/TreeBase/trunk` to `/TreeBase/leavesXform`
        cmds.rename("leavesXform")

        # get the prim
        item = ufe.GlobalSelection.get().front()
        usdPrim = stage.GetPrimAtPath(str(item.path().segments[1]))
        self.assertTrue(usdPrim)

        # the new prim name is expected to be "leavesXform1"
        assert ([x for x in stage.Traverse()] == [stage.GetPrimAtPath("/TreeBase"), 
            stage.GetPrimAtPath("/TreeBase/leavesXform"), 
            stage.GetPrimAtPath("/TreeBase/leavesXform/leaves"),
            stage.GetPrimAtPath("/TreeBase/leavesXform1"),])

    def testRenameSpecialCharacter(self):
        # open twoSpheres.ma scene in test-samples
        mayaUtils.openTwoSpheresScene()

        # clear selection to start off
        cmds.select(clear=True)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|world|usdSphereParent|usdSphereParentShape')
        usdPathSegment = usdUtils.createUfePathSegment('/sphereXform/sphere')
        basePath = ufe.Path([mayaPathSegment, usdPathSegment])
        usdSphereItem = ufe.Hierarchy.createItem(basePath)

        ufe.GlobalSelection.get().append(usdSphereItem)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # by default edit target is set to the Rootlayer.
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        # rename with special chars
        newNameWithSpecialChars = '!@#%$@$=sph^e.re_*()<>}021|'
        cmds.rename(newNameWithSpecialChars)

        # get the prim
        pSphereItem = ufe.GlobalSelection.get().front()
        usdPrim = stage.GetPrimAtPath(str(pSphereItem.path().segments[1]))
        self.assertTrue(usdPrim)

        # prim names are not allowed to have special characters except '_' 
        regex = re.compile('[@!#$%^&*()<>?/\|}{~:]')
        self.assertFalse(regex.search(usdPrim.GetName()))

        # prim names are not allowed to start with digits 
        newNameStartingWithDigit = '09123Potato'
        cmds.rename(newNameStartingWithDigit)

        self.assertFalse(usdPrim.GetName()[0].isdigit())

    def testRenameNotifications(self):
        '''Rename a USD node and test for the UFE notifications.'''

        # open usdCylinder.ma scene in test-samples
        mayaUtils.openCylinderScene()

        # clear selection to start off
        cmds.select(clear=True)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|world|mayaUsdTransform|shape')
        usdPathSegment = usdUtils.createUfePathSegment('/pCylinder1')
        cylinderPath = ufe.Path([mayaPathSegment, usdPathSegment])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        ufe.GlobalSelection.get().append(cylinderItem)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # set the edit target to the root layer
        stage.SetEditTarget(stage.GetRootLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        # Create our UFE notification observer
        ufeObs = TestObserver()

        # We start off with no observers
        self.assertFalse(ufe.Scene.hasObjectAddObserver(ufeObs))
        self.assertFalse(ufe.Scene.hasObjectDeleteObserver(ufeObs))
        self.assertFalse(ufe.Scene.hasObjectPathChangeObserver(ufeObs))

        # Add the UFE observers we want to test
        ufe.Scene.addObjectAddObserver(ufeObs)
        ufe.Scene.addObjectDeleteObserver(ufeObs)
        ufe.Scene.addObjectPathChangeObserver(ufeObs)

        # rename
        newName = 'pCylinder1_Renamed'
        cmds.rename(newName)

        # After the rename we should have 1 rename notif and no unexepected notifs.
        self.assertEqual(ufeObs.notifications(), 1)
        self.assertFalse(ufeObs.receivedUnexpectedNotif())
