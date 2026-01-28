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
import testUtils
import ufeUtils
import usdUtils
import mayaUsd_createStageWithNewLayer

import mayaUsd.ufe

from pxr import Usd, Tf, UsdShade, Sdf

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

        # Because of difference in Python binding systems (pybind11 for UFE,
        # Boost Python for mayaUsd and USD), need to pass in strings to
        # mayaUsd functions.  Multi-segment UFE paths need to have
        # comma-separated segments.
        def assertStageAndPrimAccess(proxyShapeSegment, primUfePath, primSegment):
            proxyShapePathStr = ufe.PathString.string(ufe.Path(proxyShapeSegment))

            stage     = mayaUsd.ufe.getStage(proxyShapePathStr)
            prim      = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(primUfePath))
            stagePath = mayaUsd.ufe.stagePath(stage)

            self.assertIsNotNone(stage)
            self.assertEqual(stagePath, proxyShapePathStr)
            self.assertTrue(prim.IsValid())
            self.assertEqual(str(prim.GetPath()), str(primSegment))

        assertStageAndPrimAccess(mayaSegment, ball35Path, usdSegment)

        # Rename the proxy shape node itself.  Stage and prim access should
        # still be valid, with the new path.
        mayaSegment = mayaUtils.createUfePathSegment(
            "|transform1|potato")
        cmds.rename('|transform1|proxyShape1', 'potato')
        self.assertEqual(len(cmds.ls('potato')), 1)

        ball35Path = ufe.Path([mayaSegment, usdSegment])

        assertStageAndPrimAccess(mayaSegment, ball35Path, usdSegment)

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
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

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
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

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
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

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
        # Note: each renaming does select + rename, so we need to undo four times.
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
        # Note: each renaming does select + rename, so we need to redo four times.
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
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

        # check GetLayerStack behavior
        self.assertEqual(stage.GetLayerStack()[0], stage.GetSessionLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        # expect the exception happens
        with self.assertRaises(RuntimeError):
            newName = 'Ball_35_Renamed'
            cmds.rename(newName)

    def testRenameRestrictionSessionLayer(self):
        '''
        Verify that having an opinion in the session layer does *not* prevent
        renaming a prim defined on another layer.
        '''
        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # clear the selection to start off
        cmds.select(clear=True)

        # Some values used during the test
        oldName = 'pCylinder1'
        newName = 'pCylinder1_Renamed'
        rotation = [30., 20., 10.]
        mayaPathSegment = mayaUtils.createUfePathSegment('|mayaUsdTransform|shape')

        # Some helper functions used during the test
        def getItem(name):
            itemPathSegment = usdUtils.createUfePathSegment('/' + name)
            itemPath = ufe.Path([mayaPathSegment, itemPathSegment])
            return ufe.Hierarchy.createItem(itemPath)

        def applyRotation(item, rotation):
            itemTrf = ufe.Transform3d.transform3d(item)
            itemTrf.rotate(*rotation)
            itemTrf = None

        def verifyRotation(item, rotation):
            itemTrf = ufe.Transform3d.transform3d(item)
            self.assertEqual(itemTrf.rotation(), ufe.PyUfe.Vector3d(*rotation))

        def applyRename(name):
            cmds.rename(newName)

        def verifyName(expectedName, expectedType, badName):
            # The renamed item is in the selection and has the correct name and type
            snIter = iter(ufe.GlobalSelection.get())
            item = next(snIter)
            itemName = str(item.path().back())

            self.assertEqual(itemName, expectedName)
            self.assertEqual(item.nodeType(), expectedType)

            # The renamed item is a child of its parent
            propsChildren = propsHierarchy.children()
            propsChildrenNames = [str(child.path().back()) for child in propsChildren]

            self.assertEqual(len(propsChildren), len(propsChildrenPre))
            self.assertIn(item, propsChildren)

            self.assertNotIn(badName, propsChildrenNames)
            self.assertIn(expectedName, propsChildrenNames)

        # Select a the USD cylinder object
        self.assertIsNotNone(getItem(oldName))
        cylinderItemType = getItem(oldName).nodeType()

        propsItem = ufe.Hierarchy.hierarchy(getItem(oldName)).parent()
        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildrenPre = propsHierarchy.children()

        ufe.GlobalSelection.get().append(getItem(oldName))

        # Get the USD stage
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

        # Add an opinion about the cylinder in the session layer
        stage.SetEditTarget(stage.GetSessionLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetSessionLayer())

        applyRotation(getItem(oldName), rotation)
        verifyRotation(getItem(oldName), rotation)

        # Verify the cylinder name and hierarchy before we do anything.
        verifyName(oldName, cylinderItemType, newName)

        # Rename the cylinder in the root layer
        stage.SetEditTarget(stage.GetRootLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        applyRename(newName)
        verifyName(newName, cylinderItemType, oldName)
        verifyRotation(getItem(newName), rotation)

        # Undo the rename and verify that the name and hierarchy are back to what they were
        cmds.undo()
        verifyName(oldName, cylinderItemType, newName)
        verifyRotation(getItem(oldName), rotation)

        # Redo the rename and verify that the name and hierarchy are changed again
        cmds.redo()
        verifyName(newName, cylinderItemType, oldName)
        verifyRotation(getItem(newName), rotation)


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
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

        # rename "/apple/payload/geo" to "/apple/payload/geo_renamed"
        # expect the exception happens
        with self.assertRaises(RuntimeError):
            cmds.rename("geo_renamed")

    def testRenameRestrictionDefaultPrim(self):
        '''
        Verify that a prim that is the default prim prevent
        renaming that prim when not targeting the root layer.
        '''
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('defaultPrimInSub', 'root.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)

        capsulePath = ufe.PathString.path('|stage|stageShape,/Capsule1')
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        self.assertIsNotNone(capsuleItem)
        ufe.GlobalSelection.get().append(capsuleItem)

        layer = Sdf.Layer.FindRelativeToLayer(stage.GetRootLayer(), stage.GetRootLayer().subLayerPaths[0])
        self.assertIsNotNone(layer)
        stage.SetEditTarget(layer)
        self.assertEqual(stage.GetEditTarget().GetLayer(), layer)
        
        with self.assertRaises(RuntimeError):
            cmds.rename("banana")

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
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

        # by default edit target is set to the Rootlayer.
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        # rename `/TreeBase/trunk` to `/TreeBase/leavesXform`
        cmds.rename("leavesXform")

        # get the prim
        item = ufe.GlobalSelection.get().front()
        usdPrim = stage.GetPrimAtPath(str(item.path().segments[1]))
        self.assertTrue(usdPrim)

        # the new prim name is expected to be "leavesXform1"
        def verifyNames():
            assert ([x for x in stage.Traverse()] == [stage.GetPrimAtPath("/TreeBase"), 
                stage.GetPrimAtPath("/TreeBase/leavesXform"), 
                stage.GetPrimAtPath("/TreeBase/leavesXform/leaves"),
                stage.GetPrimAtPath("/TreeBase/leavesXform1"),])

        verifyNames()

        # rename `/TreeBase/leavesXform1` to `/TreeBase/leavesXform1`: should not affect the name.
        cmds.rename("leavesXform1")

        verifyNames()

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
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

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

        # [GitHub #2150] renaming 2 usd items with illegal characters will cause Maya to crash
        cmds.file(new=True, force=True)
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        x1 = stage.DefinePrim('/Xform1', 'Xform')
        x2 = stage.DefinePrim('/Xform2', 'Xform')
        cmds.select('|stage1|stageShape1,/Xform1', replace=True)
        cmds.rename('test.') # Rename first object with illegal name
        cmds.select('|stage1|stageShape1,/Xform2', replace=True)
        cmds.rename('test.') # Rename second object with same illegal name

        # Make sure they got renamed as we expect.
        cmds.select('|stage1|stageShape1,/test_', '|stage1|stageShape1,/test_1', replace=True)
        sel = ufe.GlobalSelection.get()
        self.assertEqual(2, len(sel))

    def testRenameAutoNumber(self):
        '''Test that a trailing # is converted to a number that makes the name unique.'''

        # Create stage with two xforms.
        cmds.file(new=True, force=True)
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Xform1', 'Xform')
        stage.DefinePrim('/Xform2', 'Xform')

        # rename with trailing #
        cmds.select('|stage1|stageShape1,/Xform1', replace=True)
        cmds.rename('hello#')

        # The prim # should have been changed to a number: hello1.
        item = ufe.GlobalSelection.get().front()
        usdPrim = stage.GetPrimAtPath(str(item.path().segments[1]))
        self.assertTrue(usdPrim)
        self.assertEqual(usdPrim.GetName(), "hello1")

        cmds.select('|stage1|stageShape1,/Xform2', replace=True)
        cmds.rename('hello#')

        # The prim # should have been changed to a unique number: hello2.
        item = ufe.GlobalSelection.get().front()
        usdPrim = stage.GetPrimAtPath(str(item.path().segments[1]))
        self.assertTrue(usdPrim)
        self.assertEqual(usdPrim.GetName(), "hello2")

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
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

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
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

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
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

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

    def testRelationshipAndConnectionUpdatesOnRename(self):

        '''
        Verify that relationship targets and connection sources are properly
        updated when renaming.
        '''

        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene("MaterialX", "MtlxUVStreamTest.usda")
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        def testPaths(self, shapeStage, cubeName):
            # Test the Material assignment relationship on the Mesh:
            pCube1Binding = UsdShade.MaterialBindingAPI(shapeStage.GetPrimAtPath('/{}'.format(cubeName)))
            self.assertEqual(pCube1Binding.GetDirectBinding().GetMaterialPath(), Sdf.Path("/{}/Looks/standardSurface2SG".format(cubeName)))

            # Test one connection inside the Material:
            mix1Shader = UsdShade.ConnectableAPI(shapeStage.GetPrimAtPath('/{}/Looks/standardSurface2SG/MayaNG_standardSurface2SG/mix1'.format(cubeName)))
            fgInput = mix1Shader.GetInput("fg")
            srcConnectAPI = fgInput.GetConnectedSource()[0]
            self.assertEqual(srcConnectAPI.GetPath(), Sdf.Path("/{}/Looks/standardSurface2SG/MayaNG_standardSurface2SG/ramp1".format(cubeName)))

        testPaths(self, shapeStage, "pCube1")

        # Rename /pCube1 to /banana
        cubePath = ufe.PathString.path('|stage|stageShape,/pCube1')
        cubeItem = ufe.Hierarchy.createItem(cubePath)
        ufe.GlobalSelection.get().append(cubeItem)
        cmds.rename("banana")

        testPaths(self, shapeStage, "banana")

        cmds.undo()

        testPaths(self, shapeStage, "pCube1")

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testUfeRenameCommandAPI(self):
        '''Test that the rename command can be invoked using the 3 known APIs.'''

        testFile = testUtils.getTestScene('MaterialX', 'BatchOpsTestScene.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        # Test NoExecute API:
        geomItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane1')
        self.assertIsNotNone(geomItem)

        renameCmd = ufe.SceneItemOps.sceneItemOps(geomItem).renameItemCmdNoExecute(ufe.PathComponent("carotte"))
        self.assertIsNotNone(renameCmd)
        renameCmd.execute()

        nonExistentItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane1')
        self.assertIsNone(nonExistentItem)

        carotteItem = ufeUtils.createUfeSceneItem(shapeNode, '/carotte')
        self.assertIsNotNone(carotteItem)
        self.assertEqual(carotteItem, renameCmd.sceneItem)

        renameCmd.undo()

        geomItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane1')
        self.assertIsNotNone(geomItem)
        nonExistentItem = ufeUtils.createUfeSceneItem(shapeNode, '/carotte')
        self.assertIsNone(nonExistentItem)

        renameCmd.redo()

        nonExistentItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane1')
        self.assertIsNone(nonExistentItem)

        carotteItem = ufeUtils.createUfeSceneItem(shapeNode, '/carotte')
        self.assertIsNotNone(carotteItem)
        self.assertEqual(carotteItem, renameCmd.sceneItem)

        renameCmd.undo()

        # Test Exec but undoable API:
        geomItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane1')
        self.assertIsNotNone(geomItem)

        renameCmd = ufe.SceneItemOps.sceneItemOps(geomItem).renameItemCmd(ufe.PathComponent("carotte"))
        self.assertIsNotNone(renameCmd)

        nonExistentItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane1')
        self.assertIsNone(nonExistentItem)

        carotteItem = ufeUtils.createUfeSceneItem(shapeNode, '/carotte')
        self.assertIsNotNone(carotteItem)
        self.assertEqual(carotteItem, renameCmd.item)

        renameCmd.undoableCommand.undo()

        geomItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane1')
        self.assertIsNotNone(geomItem)
        nonExistentItem = ufeUtils.createUfeSceneItem(shapeNode, '/carotte')
        self.assertIsNone(nonExistentItem)

        renameCmd.undoableCommand.redo()

        nonExistentItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane1')
        self.assertIsNone(nonExistentItem)

        carotteItem = ufeUtils.createUfeSceneItem(shapeNode, '/carotte')
        self.assertIsNotNone(carotteItem)
        self.assertEqual(carotteItem, renameCmd.item)

        renameCmd.undoableCommand.undo()

        # Test non-undoable API:
        geomItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane1')
        self.assertIsNotNone(geomItem)

        renamedItem = ufe.SceneItemOps.sceneItemOps(geomItem).renameItem(ufe.PathComponent("carotte"))
        self.assertIsNotNone(renameCmd)

        nonExistentItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane1')
        self.assertIsNone(nonExistentItem)

        carotteItem = ufeUtils.createUfeSceneItem(shapeNode, '/carotte')
        self.assertIsNotNone(carotteItem)
        self.assertEqual(carotteItem, renamedItem)

    def testRenameRestrictionMutedLayer(self):
        '''
        Test rename restriction - we don't allow renaming a prim
        when there are opinions on a muted layer.
        '''
        
        # Create a stage
        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer

        proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()
        self.assertTrue(stage)

        # Helpers
        def createLayer(index):
            layer = Sdf.Layer.CreateAnonymous()
            stage.GetRootLayer().subLayerPaths.append(layer.identifier)
            return layer

        def targetSubLayer(layer):
            stage.SetEditTarget(layer)
            self.assertEqual(stage.GetEditTarget().GetLayer(), layer)
            layer = None

        def muteSubLayer(layer):
            # Note: mute by passing through the stage, otherwise the stage won't get recomposed
            stage.MuteLayer(layer.identifier)

        def setSphereRadius(radius):
            spherePrim = stage.GetPrimAtPath('/A/ball')
            spherePrim.GetAttribute('radius').Set(radius)
            spherePrim = None

        # Add two new layers
        topLayer = createLayer(0)
        bottomLayer = createLayer(1)

        # Create a xform prim named A and a sphere on the bottom layer
        targetSubLayer(bottomLayer)
        stage.DefinePrim('/A', 'Xform')
        stage.DefinePrim('/A/ball', 'Sphere')
        setSphereRadius(7.12)

        targetSubLayer(topLayer)
        setSphereRadius(4.32)
        
        # Set target to bottom layer and mute the top layer
        targetSubLayer(bottomLayer)
        muteSubLayer(topLayer)

        # Try to rename the prim with muted opinion: it should fail
        with self.assertRaises(RuntimeError):
            cmds.rename('%s,/A/ball' % proxyShapePathStr, 'ball2')

        self.assertTrue(stage.GetPrimAtPath('/A/ball'))
        self.assertFalse(stage.GetPrimAtPath('/A/group1/ball'))
        
    def testGroupRenamingMaxSuffix(self):
        # '''
        # Test that when creating a new group, it checks ALL CHILDREN to find the max index,
        # then increments by one. This matches Maya's behavior where renaming a group to
        # "group10" and then grouping again should create "group11", not "group1".
        # '''

        # Open ballset.ma scene in testSamples
        mayaUtils.openGroupBallsScene()

        # Clear selection to start off
        cmds.select(clear=True)

        mayaPathSegment = mayaUtils.createUfePathSegment(
            "|transform1|proxyShape1")

        usdSegmentBall1 = usdUtils.createUfePathSegment(
            "/Ball_set/Props/Ball_1")
        ball1Path = ufe.Path([mayaPathSegment, usdSegmentBall1])
        ball1Item = ufe.Hierarchy.createItem(ball1Path)
        
        usdSegmentBall2 = usdUtils.createUfePathSegment(
            "/Ball_set/Props/Ball_2")
        ball2Path = ufe.Path([mayaPathSegment, usdSegmentBall2])
        ball2Item = ufe.Hierarchy.createItem(ball2Path)
        
        usdSegmentBall3 = usdUtils.createUfePathSegment(
            "/Ball_set/Props/Ball_3")
        ball3Path = ufe.Path([mayaPathSegment, usdSegmentBall3])
        ball3Item = ufe.Hierarchy.createItem(ball3Path)

        usdSegmentBall4 = usdUtils.createUfePathSegment(
            "/Ball_set/group3/Props/Ball_4")
        ball4Path = ufe.Path([mayaPathSegment, usdSegmentBall4])
        ball4Item = ufe.Hierarchy.createItem(ball4Path)
        
        usdSegmentBall5 = usdUtils.createUfePathSegment(
            "/Ball_set/Props/Ball_5")
        ball5Path = ufe.Path([mayaPathSegment, usdSegmentBall5])
        ball5Item = ufe.Hierarchy.createItem(ball5Path)

        usdSegmentProps = usdUtils.createUfePathSegment("/Ball_set/Props")
        parentPath = ufe.Path([mayaPathSegment, usdSegmentProps])
        parentItem = ufe.Hierarchy.createItem(parentPath)

        parentHierarchy = ufe.Hierarchy.hierarchy(parentItem)
        parentChildrenPre = parentHierarchy.children()
        self.assertEqual(len(parentChildrenPre), 6)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

        # set the edit target to balls.usda
        layer = stage.GetLayerStack()[1]
        self.assertEqual("ballset.usda", layer.GetDisplayName())
        stage.SetEditTarget(layer)

        # group ball3 and ball5 together
        if ufeUtils.ufeFeatureSetVersion() >= 3:
            globalSn = ufe.GlobalSelection.get()
            globalSn.append(ball5Item)
            globalSn.append(ball3Item)

            cmds.group(ufe.PathString.string(ball5Path), 
                                   ufe.PathString.string(ball3Path))
        else:
            ufeSelectionList = ufe.Selection()
            ufeSelectionList.append(ball5Item)
            ufeSelectionList.append(ball3Item)

            groupCmd = parentHierarchy.createGroupCmd(ufeSelectionList)
            groupCmd.execute()

        cmds.select(clear=True)
        
        # Group ball1 and ball2 together
        if ufeUtils.ufeFeatureSetVersion() >= 3:
            globalSn = ufe.GlobalSelection.get()
            globalSn.append(ball1Item)
            globalSn.append(ball2Item)

            cmds.group(ufe.PathString.string(ball1Path), 
                       ufe.PathString.string(ball2Path))
        else:
            ufeSelectionList = ufe.Selection()
            ufeSelectionList.append(ball1Item)
            ufeSelectionList.append(ball2Item)

            groupCmd = parentHierarchy.createGroupCmd(ufeSelectionList)
            groupCmd.execute()

        # Group object (a.k.a parent) will be added to selection list. This behavior matches the native Maya group command.
        globalSelection = ufe.GlobalSelection.get()

        # Verify group1 exists and has the correct name
        group1Path = ufe.Path([mayaPathSegment, usdUtils.createUfePathSegment("/Ball_set/Props/group1")])
        group1Item = ufe.Hierarchy.createItem(group1Path)
        self.assertEqual(str(group1Item.path().back()), "group1")
        
        # Verify group2 exists, is in the selection (last created group), and has the correct name
        group2Path = ufe.Path([mayaPathSegment, usdUtils.createUfePathSegment("/Ball_set/Props/group2")])
        group2Item = ufe.Hierarchy.createItem(group2Path)
        self.assertEqual(globalSelection.front(), group2Item)
        self.assertEqual(str(group2Item.path().back()), "group2")

        usdSegmentProps= usdUtils.createUfePathSegment(
            "/Ball_set/Props")
        propsPath = ufe.Path([mayaPathSegment, usdSegmentProps])
        propsItem = ufe.Hierarchy.createItem(propsPath)

        # Group Props together
        if ufeUtils.ufeFeatureSetVersion() >= 3:
            globalSn = ufe.GlobalSelection.get()
            globalSn.append(propsItem)

            cmds.group(ufe.PathString.string(propsPath))
        else:
            ufeSelectionList = ufe.Selection()
            ufeSelectionList.append(propsItem)

            groupCmd = parentHierarchy.createGroupCmd(ufeSelectionList)
            groupCmd.execute()

        # Verify the new group taskes the largest suffix found amongst other groups of the  same name and increments it by 1.
        group3Path = ufe.Path([mayaPathSegment, usdUtils.createUfePathSegment("/Ball_set/group3")])
        group3Item = ufe.Hierarchy.createItem(group3Path)
        self.assertEqual(str(group3Item.path().back()), "group3")

        # Rename group1 to group2, which should result in group3 since group2 already exists within this layer.
        group1PathAfterGrouping = ufe.Path([mayaPathSegment, usdUtils.createUfePathSegment("/Ball_set/group3/Props/group1")])
        group1ItemAfterGrouping = ufe.Hierarchy.createItem(group1PathAfterGrouping)
        self.assertIsNotNone(group1ItemAfterGrouping, "group1 should exist after grouping Props")
        cmds.select(clear=True)
        ufe.GlobalSelection.get().append(group1ItemAfterGrouping)
        cmds.rename("group2")
        
        # Verify it was renamed to group3 (max suffix at this layer+children is 2, so 2+1=3)
        snIter = iter(ufe.GlobalSelection.get())
        renamedGroupItem = next(snIter)
        renamedGroupName = str(renamedGroupItem.path().back())
        self.assertEqual(renamedGroupName, "group3")

        # Renaming the highest group3 to group3 should keep the 3 suffix as it is
        cmds.select(clear=True)
        ufe.GlobalSelection.get().append(group3Item)
        cmds.rename("group3")
        
        # Verify it remains group3 (no change when renaming to the same name)
        snIter = iter(ufe.GlobalSelection.get())
        unchangedGroupItem = next(snIter)
        unchangedGroupName = str(unchangedGroupItem.path().back())
        self.assertEqual(unchangedGroupName, "group3")
        self.assertEqual(unchangedGroupItem, group3Item)

        # Rename props group3 to group5
        cmds.select(clear=True)
        ufe.GlobalSelection.get().append(renamedGroupItem)
        cmds.rename("group5")
        
        # Verify it was renamed to group5
        snIter = iter(ufe.GlobalSelection.get())
        renamedToGroup5Item = next(snIter)
        renamedToGroup5Name = str(renamedToGroup5Item.path().back())
        self.assertEqual(renamedToGroup5Name, "group5")

        # Get the parent hierarchy after grouping Props
        propsPathAfterGrouping = ufe.Path([mayaPathSegment, usdUtils.createUfePathSegment("/Ball_set/group3/Props")])
        propsItemAfterGrouping = ufe.Hierarchy.createItem(propsPathAfterGrouping)
        propsHierarchyAfterGrouping = ufe.Hierarchy.hierarchy(propsItemAfterGrouping)
        
        # Group ball4, should get name group6
        cmds.select(clear=True)
        if ufeUtils.ufeFeatureSetVersion() >= 3:
            globalSn = ufe.GlobalSelection.get()
            globalSn.append(ball4Item)
            cmds.group(ufe.PathString.string(ball4Path))
        else:
            ufeSelectionList = ufe.Selection()
            ufeSelectionList.append(ball4Item)
            groupCmd = propsHierarchyAfterGrouping.createGroupCmd(ufeSelectionList)
            groupCmd.execute()
        
        # Verify group6 was created (max suffix is 5, so 5+1=6)
        # The group command adds the created group to the selection
        globalSelection = ufe.GlobalSelection.get()
        group6Item = globalSelection.front()
        self.assertIsNotNone(group6Item, "Group should be created and added to selection")
        group6Name = str(group6Item.path().back())
        self.assertEqual(group6Name, "group6")
        
        # Also verify we can access it by path
        group6Path = ufe.Path([mayaPathSegment, usdUtils.createUfePathSegment("/Ball_set/group3/Props/group6")])
        group6ItemByPath = ufe.Hierarchy.createItem(group6Path)
        self.assertIsNotNone(group6ItemByPath, "Group6 should be accessible by path")
        self.assertEqual(str(group6ItemByPath.path().back()), "group6")

        # Rename group2 to group5, should get group7 (since group5 and group6 already exist)
        group2PathAfterGrouping = ufe.Path([mayaPathSegment, usdUtils.createUfePathSegment("/Ball_set/group3/Props/group2")])
        group2ItemAfterGrouping = ufe.Hierarchy.createItem(group2PathAfterGrouping)
        cmds.select(clear=True)
        ufe.GlobalSelection.get().append(group2ItemAfterGrouping)
        cmds.rename("group5")
        
        # Verify it was renamed to group7 (max suffix at this layer+children is 6, so 6+1=7)
        snIter = iter(ufe.GlobalSelection.get())
        renamedToGroup7Item = next(snIter)
        renamedToGroup7Name = str(renamedToGroup7Item.path().back())
        self.assertEqual(renamedToGroup7Name, "group7")

        # Rename group6 to group6 again, should keep the single increment (case for excluding self)
        # This tests that when renaming to the same name, it excludes itself from the max calculation
        cmds.select(clear=True)
        ufe.GlobalSelection.get().append(group6Item)
        cmds.rename("group6")
        
        # Verify it remains group6 (when renaming to same name, excludes self from max calculation)
        snIter = iter(ufe.GlobalSelection.get())
        unchangedGroup6Item = next(snIter)
        unchangedGroup6Name = str(unchangedGroup6Item.path().back())
        self.assertEqual(unchangedGroup6Name, "group6")
        self.assertEqual(unchangedGroup6Item, group6Item)