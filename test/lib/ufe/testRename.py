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

import fixturesUtils
import mayaUtils
import usdUtils

import mayaUsd.ufe

from pxr import Usd, Tf

from maya import cmds
from maya import standalone

import ufe

import collections
import os
import re
import unittest


# This class is used for composition arcs query.
class CompositionQuery(object):
    def __init__(self, prim):
        super(CompositionQuery, self).__init__()
        self._prim = prim

    def _getDict(self, arc):
        dic = collections.OrderedDict()

        dic['ArcType'] = str(arc.GetArcType().displayName)
        dic['nodePath'] = str(arc.GetTargetNode().path)

        return dic

    def getData(self):
        query = Usd.PrimCompositionQuery(self._prim)
        arcValueDicList = [self._getDict(arc) for arc in query.GetCompositionArcs()]
        return arcValueDicList

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
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    @classmethod
    def tearDownClass(cls):
        cmds.file(new=True, force=True)

        standalone.uninitialize()

    def setUp(self):
        ''' Called initially to set up the Maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)
        
        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()
        
        # Clear selection to start off
        cmds.select(clear=True)

    def testRenameProxyShape(self):
        '''Rename proxy shape, UFE lookup should succeed.'''

        mayaSegment = mayaUtils.createUfePathSegment(
            "|transform1|proxyShape1")
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
            "|transform1|potato")
        cmds.rename('|transform1|proxyShape1', 'potato')
        self.assertEqual(len(cmds.ls('potato')), 1)

        ball35Path = ufe.Path([mayaSegment, usdSegment])
        ball35PathStr = ','.join(
            [str(segment) for segment in ball35Path.segments])

        assertStageAndPrimAccess(mayaSegment, ball35PathStr, usdSegment)

    def testRename(self):
        '''
        Testing renaming a USD node.
        '''
        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # clear selection to start off
        cmds.select(clear=True)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
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
        primspec = stage.GetEditTarget().GetPrimSpecForScenePath(defaultPrim.GetPath())

        # set primspec name
        primspec.name = "TreeBase_potato"

        # get the renamed prim
        renamedPrim = stage.GetPrimAtPath('/TreeBase_potato')

        # One must use the SdfLayer API for setting the defaultPrim when you rename the prim it identifies.
        stage.SetDefaultPrim(renamedPrim)

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
        '''
        Testing rename USD node undo/redo.
        '''

        # open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # clear selection to start off
        cmds.select(clear=True)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|mayaUsdTransform|shape')
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

    def testRenamePreserveLoadRules(self):
        '''
        Testing that renaming a USD node preserve load rules targeting it.
        '''

        # open scene in testSamples. Relevant hierarchy is:
        #    |transform1
        #       |proxyShape1
        #          /Ball_set
        #             /Props
        #                /Ball_1
        #                /Ball_2
        #                ...

        mayaUtils.openGroupBallsScene()

        # USD paths used in the test.
        propsSdfPath = '/Ball_set/Props'
        ball1SdfPath = '/Ball_set/Props/Ball_1'
        ball2SdfPath = '/Ball_set/Props/Ball_2'

        ball1SdfRenamedPath = '/Ball_set/Props/Round_1'
        ball2SdfRenamedPath = '/Ball_set/Props/Round_2'

        # Maya UFE segment and USD stage, needed in various places below.
        mayaPathSegment = mayaUtils.createUfePathSegment('|transform1|proxyShape1')
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # Setup the load rules:
        #     /Props is unloaded
        #     /Props/Ball1 is loaded
        #     /Props/Ball2 has no rule, so is governed by /Props
        loadRules = stage.GetLoadRules()
        loadRules.AddRule(propsSdfPath, loadRules.NoneRule)
        loadRules.AddRule(ball1SdfPath, loadRules.AllRule)
        stage.SetLoadRules(loadRules)

        self.assertEqual(loadRules.OnlyRule, stage.GetLoadRules().GetEffectiveRuleForPath(propsSdfPath))
        self.assertEqual(loadRules.AllRule, stage.GetLoadRules().GetEffectiveRuleForPath(ball1SdfPath))
        self.assertEqual(loadRules.NoneRule, stage.GetLoadRules().GetEffectiveRuleForPath(ball2SdfPath))

        def childrenNames(children):
           return [str(child.path().back()) for child in children]

        # Verify we can find the expected prims.
        def verifyNames(expectedNames):
            propsPathSegment = usdUtils.createUfePathSegment(propsSdfPath)
            propsUfePath = ufe.Path([mayaPathSegment, propsPathSegment])
            propsUfeItem = ufe.Hierarchy.createItem(propsUfePath)
            propsHierarchy = ufe.Hierarchy.hierarchy(propsUfeItem)
            propsChildren = propsHierarchy.children()
            propsChildrenNames = childrenNames(propsChildren)
            for name in expectedNames:
                self.assertIn(name, propsChildrenNames)

        verifyNames(['Ball_1', 'Ball_2'])

        # Rename the balls.
        def rename(sdfPath, newName):
            ufePathSegment = usdUtils.createUfePathSegment(sdfPath)
            ufePath = ufe.Path([mayaPathSegment, ufePathSegment])
            ufeItem = ufe.Hierarchy.createItem(ufePath)
            cmds.select(clear=True)
            ufe.GlobalSelection.get().append(ufeItem)
            cmds.rename(newName)

        rename(ball1SdfPath, "Round_1")
        rename(ball2SdfPath, "Round_2")

        # Verify we can find the expected renamed prims.
        verifyNames(['Round_1', 'Round_2'])

        # Verify the load rules for each item has not changed.
        self.assertEqual(loadRules.OnlyRule, stage.GetLoadRules().GetEffectiveRuleForPath(propsSdfPath))
        self.assertEqual(loadRules.AllRule, stage.GetLoadRules().GetEffectiveRuleForPath(ball1SdfRenamedPath))
        self.assertEqual(loadRules.NoneRule, stage.GetLoadRules().GetEffectiveRuleForPath(ball2SdfRenamedPath))

        # Undo both rename commands and re-verify load rules.
        # Note: each renaming does slect + rename, so we need to undo four times.
        cmds.undo()
        cmds.undo()
        cmds.undo()
        cmds.undo()

        # Verify we can find the expected renamed prims.
        verifyNames(['Ball_1', 'Ball_2'])

        # Verify the load rules for each item has not changed.
        self.assertEqual(loadRules.OnlyRule, stage.GetLoadRules().GetEffectiveRuleForPath(propsSdfPath))
        self.assertEqual(loadRules.AllRule, stage.GetLoadRules().GetEffectiveRuleForPath(ball1SdfPath))
        self.assertEqual(loadRules.NoneRule, stage.GetLoadRules().GetEffectiveRuleForPath(ball2SdfPath))

        # Redo both rename commands and re-verify load rules.
        # Note: each renaming does slect + rename, so we need to redo four times.
        cmds.redo()
        cmds.redo()
        cmds.redo()
        cmds.redo()
        
        # Verify we can find the expected renamed prims.
        verifyNames(['Round_1', 'Round_2'])

        # Verify the load rules for each item has not changed.
        self.assertEqual(loadRules.OnlyRule, stage.GetLoadRules().GetEffectiveRuleForPath(propsSdfPath))
        self.assertEqual(loadRules.AllRule, stage.GetLoadRules().GetEffectiveRuleForPath(ball1SdfRenamedPath))
        self.assertEqual(loadRules.NoneRule, stage.GetLoadRules().GetEffectiveRuleForPath(ball2SdfRenamedPath))

    def testRenameRestrictionSameLayerDef(self):
        '''Restrict renaming USD node. Cannot rename a prim defined on another layer.'''

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|transform1|proxyShape1')
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

    def testRenameRestrictionHasSpecs(self):
        '''Restrict renaming USD node. Cannot rename a node that doesn't contribute to the final composed prim'''

        # open appleBite.ma scene in testSamples
        mayaUtils.openAppleBiteScene()

        # clear selection to start off
        cmds.select(clear=True)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Asset_flattened_instancing_and_class_removed_usd|Asset_flattened_instancing_and_class_removed_usdShape')
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
        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # clear selection to start off
        cmds.select(clear=True)

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
        # open twoSpheres.ma scene in testSamples
        mayaUtils.openTwoSpheresScene()

        # clear selection to start off
        cmds.select(clear=True)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|usdSphereParent|usdSphereParentShape')
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

        # rename starting with digits.
        newNameStartingWithDigit = '09123Potato'
        self.assertFalse(Tf.IsValidIdentifier(newNameStartingWithDigit))
        cmds.rename(newNameStartingWithDigit)

        # get the prim
        pSphereItem = ufe.GlobalSelection.get().front()
        usdPrim = stage.GetPrimAtPath(str(pSphereItem.path().segments[1]))
        self.assertTrue(usdPrim)

        # prim names are not allowed to start with digits
        newValidName = Tf.MakeValidIdentifier(newNameStartingWithDigit)
        self.assertEqual(usdPrim.GetName(), newValidName)

    def testRenameNotifications(self):
        '''Rename a USD node and test for the UFE notifications.'''

        # open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # clear selection to start off
        cmds.select(clear=True)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|mayaUsdTransform|shape')
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
        self.assertFalse(ufe.Scene.hasObserver(ufeObs))

        # Add the UFE observer we want to test
        ufe.Scene.addObserver(ufeObs)

        # rename
        newName = 'pCylinder1_Renamed'
        cmds.rename(newName)

        # After the rename we should have 1 rename notif and no unexepected notifs.
        self.assertEqual(ufeObs.notifications(), 1)
        self.assertFalse(ufeObs.receivedUnexpectedNotif())

    def testRenameRestrictionVariant(self):

        '''Renaming prims inside a variantSet is not allowed.'''

        cmds.file(new=True, force=True)

        # open Variant.ma scene in testSamples
        mayaUtils.openVariantSetScene()

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

        # rename "/objects/Geom" to "/objects/Geom_something"
        # expect the exception happens
        with self.assertRaises(RuntimeError):
            cmds.rename("Geom_something")

        # clear selection
        cmds.select(clear=True)

        # add Small_Potato to selection list 
        ufe.GlobalSelection.get().append(smallPotatoItem)

        # get prim spec for Small_Potato prim
        primspecSmallPotato = stage.GetEditTarget().GetPrimSpecForScenePath(smallPotatoPrim.GetPath());

        # primSpec is expected to be None
        self.assertIsNone(primspecSmallPotato)

        # rename "/objects/Geom/Small_Potato" to "/objects/Geom/Small_Potato_something"
        # expect the exception happens
        with self.assertRaises(RuntimeError):
            cmds.rename("Small_Potato_something")

    def testAutomaticRenameCompArcs(self):

        '''
        Verify that SdfPath update happens automatically for "internal reference", 
        "inherit", "specialize" after the rename.
        '''
        cmds.file(new=True, force=True)

        # open compositionArcs.ma scene in testSamples
        mayaUtils.openCompositionArcsScene()

        # stage
        mayaPathSegment = mayaUtils.createUfePathSegment('|CompositionArcs_usd|CompositionArcs_usdShape')
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # first check for ArcType, nodePath values. I am only interested in the composition Arc other than "root".
        compQueryPrimA = CompositionQuery(stage.GetPrimAtPath('/objects/geos/cube_A'))
        self.assertTrue(list(compQueryPrimA.getData()[1].values()), ['reference', '/objects/geos/cube'])

        compQueryPrimB = CompositionQuery(stage.GetPrimAtPath('/objects/geos/cube_B'))
        self.assertTrue(list(compQueryPrimB.getData()[1].values()), ['inherit', '/objects/geos/cube'])

        compQueryPrimC = CompositionQuery(stage.GetPrimAtPath('/objects/geos/cube_C'))
        self.assertTrue(list(compQueryPrimC.getData()[1].values()), ['specialize', '/objects/geos/cube'])

        # rename /objects/geos/cube ---> /objects/geos/cube_banana
        cubePath = ufe.PathString.path('|CompositionArcs_usd|CompositionArcs_usdShape,/objects/geos/cube')
        cubeItem = ufe.Hierarchy.createItem(cubePath)
        ufe.GlobalSelection.get().append(cubeItem)
        cmds.rename("cube_banana")

        # check for ArcType, nodePath values again. 
        # expect nodePath to be updated for all the arc compositions
        compQueryPrimA = CompositionQuery(stage.GetPrimAtPath('/objects/geos/cube_A'))
        self.assertTrue(list(compQueryPrimA.getData()[1].values()), ['reference', '/objects/geos/cube_banana'])

        compQueryPrimB = CompositionQuery(stage.GetPrimAtPath('/objects/geos/cube_B'))
        self.assertTrue(list(compQueryPrimB.getData()[1].values()), ['inherit', '/objects/geos/cube_banana'])

        compQueryPrimC = CompositionQuery(stage.GetPrimAtPath('/objects/geos/cube_C'))
        self.assertTrue(list(compQueryPrimC.getData()[1].values()), ['specialize', '/objects/geos/cube_banana'])

        # rename /objects/geos ---> /objects/geos_cucumber
        geosPath = ufe.PathString.path('|CompositionArcs_usd|CompositionArcs_usdShape,/objects/geos')
        geosItem = ufe.Hierarchy.createItem(geosPath)
        cmds.select(clear=True)
        ufe.GlobalSelection.get().append(geosItem)
        cmds.rename("geos_cucumber")

        # check for ArcType, nodePath values again. 
        # expect nodePath to be updated for all the arc compositions
        compQueryPrimA = CompositionQuery(stage.GetPrimAtPath('/objects/geos_cucumber/cube_A'))
        self.assertTrue(list(compQueryPrimA.getData()[1].values()), ['reference', '/objects/geos_cucumber/cube_banana'])

        compQueryPrimB = CompositionQuery(stage.GetPrimAtPath('/objects/geos_cucumber/cube_B'))
        self.assertTrue(list(compQueryPrimB.getData()[1].values()), ['inherit', '/objects/geos_cucumber/cube_banana'])

        compQueryPrimC = CompositionQuery(stage.GetPrimAtPath('/objects/geos_cucumber/cube_C'))
        self.assertTrue(list(compQueryPrimC.getData()[1].values()), ['specialize', '/objects/geos_cucumber/cube_banana'])

        # rename /objects ---> /objects_eggplant
        objectsPath = ufe.PathString.path('|CompositionArcs_usd|CompositionArcs_usdShape,/objects')
        objectsItem = ufe.Hierarchy.createItem(objectsPath)
        cmds.select(clear=True)
        ufe.GlobalSelection.get().append(objectsItem)
        cmds.rename("objects_eggplant")

        # check for ArcType, nodePath values again. 
        # expect nodePath to be updated for all the arc compositions
        compQueryPrimA = CompositionQuery(stage.GetPrimAtPath('/objects_eggplant/geos_cucumber/cube_A'))
        self.assertTrue(list(compQueryPrimA.getData()[1].values()), ['reference', '/objects_eggplant/geos_cucumber/cube_banana'])

        compQueryPrimB = CompositionQuery(stage.GetPrimAtPath('/objects_eggplant/geos_cucumber/cube_B'))
        self.assertTrue(list(compQueryPrimB.getData()[1].values()), ['inherit', '/objects_eggplant/geos_cucumber/cube_banana'])

        compQueryPrimC = CompositionQuery(stage.GetPrimAtPath('/objects_eggplant/geos_cucumber/cube_C'))
        self.assertTrue(list(compQueryPrimC.getData()[1].values()), ['specialize', '/objects_eggplant/geos_cucumber/cube_banana'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
