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

import mayaUsd.ufe

from pxr import Kind, Usd, UsdGeom, Vt

from maya import cmds
from maya import standalone

import ufe

import os
import unittest

def createStage():
    ''' create a simple stage '''
    cmds.file(new=True, force=True)
    import mayaUsd_createStageWithNewLayer
    proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
    proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
    proxyShapePath = proxyShapes[0]
    proxyShapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(proxyShapePath))
    proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
    stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
    return (stage, proxyShapePathStr, proxyShapeItem, proxyShapeContextOps)

class SphereGenerator():
    ''' simple sphere generator '''
    def __init__(self, num, contextOp, proxyShapePathStr):
        self.gen = self.__generate(num)
        self.num = num
        self.contextOp = contextOp
        self.proxyShapePathStr = proxyShapePathStr

    def createSphere(self):
        return next(self.gen)

    def __addPrimSphere(self, increment):
        self.contextOp.doOp(['Add New Prim', 'Sphere'])
        return ufe.PathString.path('{},/Sphere{}'.format(self.proxyShapePathStr, increment))

    def __generate(self, num):
        increment = 0
        while increment < self.num:
            increment += 1
            yield self.__addPrimSphere(increment)

class GroupCmdTestCase(unittest.TestCase):
    '''Verify the Maya group command, for multiple runtimes.

    As of 19-Nov-2018, the UFE group command is not integrated into Maya, so
    directly test the UFE undoable command.
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

        # Open ballset.ma scene in testSamples
        mayaUtils.openGroupBallsScene()

        # Clear selection to start off
        cmds.select(clear=True)

    def testUsdGroup(self):
        '''Creation of USD group objects.'''

        mayaPathSegment = mayaUtils.createUfePathSegment(
            "|transform1|proxyShape1")

        usdSegmentBall5 = usdUtils.createUfePathSegment(
            "/Ball_set/Props/Ball_5")
        ball5Path = ufe.Path([mayaPathSegment, usdSegmentBall5])
        ball5Item = ufe.Hierarchy.createItem(ball5Path)

        usdSegmentBall3 = usdUtils.createUfePathSegment(
            "/Ball_set/Props/Ball_3")
        ball3Path = ufe.Path([mayaPathSegment, usdSegmentBall3])
        ball3Item = ufe.Hierarchy.createItem(ball3Path)

        usdSegmentProps = usdUtils.createUfePathSegment("/Ball_set/Props")
        parentPath = ufe.Path([mayaPathSegment, usdSegmentProps])
        parentItem = ufe.Hierarchy.createItem(parentPath)

        parentHierarchy = ufe.Hierarchy.hierarchy(parentItem)
        parentChildrenPre = parentHierarchy.children()
        self.assertEqual(len(parentChildrenPre), 6)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # set the edit target to balls.usda
        layer = stage.GetLayerStack()[1]
        self.assertEqual("ballset.usda", layer.GetDisplayName())
        stage.SetEditTarget(layer)

        if ufeUtils.ufeFeatureSetVersion() >= 3:
            globalSn = ufe.GlobalSelection.get()
            globalSn.append(ball5Item)
            globalSn.append(ball3Item)

            # group
            groupName = cmds.group(ufe.PathString.string(ball5Path), 
                                   ufe.PathString.string(ball3Path), n="newGroup")
        else:
            newGroupName = ufe.PathComponent("newGroup")

            ufeSelectionList = ufe.Selection()
            ufeSelectionList.append(ball5Item)
            ufeSelectionList.append(ball3Item)

            groupCmd = parentHierarchy.createGroupCmd(ufeSelectionList, newGroupName)
            groupCmd.execute()

        # Group object (a.k.a parent) will be added to selection list. This behavior matches the native Maya group command.
        globalSelection = ufe.GlobalSelection.get()

        groupPath = ufe.Path([mayaPathSegment, usdUtils.createUfePathSegment("/Ball_set/Props/newGroup1")])
        self.assertEqual(globalSelection.front(), ufe.Hierarchy.createItem(groupPath))

        # Group object (a.k.a parent) will be added to selection list. This behavior matches the native Maya group command.
        globalSelection = ufe.GlobalSelection.get()

        groupPath = ufe.Path([mayaPathSegment, usdUtils.createUfePathSegment("/Ball_set/Props/newGroup1")])
        self.assertEqual(globalSelection.front(), ufe.Hierarchy.createItem(groupPath))

        parentChildrenPost = parentHierarchy.children()
        self.assertEqual(len(parentChildrenPost), 5)

        # The command will now append a number 1 at the end to match the naming
        # convention in Maya.
        if ufeUtils.ufeFeatureSetVersion() >= 3:
            newGroupPath = parentPath + ufe.PathComponent(groupName)
        else:
            newGroupPath = parentPath + ufe.PathComponent("newGroup1")

        # Make sure the new group item has the correct Usd type
        newGroupItem = ufe.Hierarchy.createItem(newGroupPath)
        newGroupPrim = usdUtils.getPrimFromSceneItem(newGroupItem)
        newGroupType = newGroupPrim.GetTypeName()
        self.assertEqual(newGroupType, 'Xform')

        childPaths = set([child.path() for child in parentChildrenPost])

        self.assertTrue(newGroupPath in childPaths)
        self.assertTrue(ball5Path not in childPaths)
        self.assertTrue(ball3Path not in childPaths)
        
        if ufeUtils.ufeFeatureSetVersion() >= 3:
            cmds.undo()
        else:
            groupCmd.undo()

        # global selection should not be empty after undo.
        if ufeUtils.ufeFeatureSetVersion() >= 3:
            self.assertEqual(len(globalSelection), 2)
        else:
            self.assertEqual(len(globalSelection), 1)

        parentChildrenUndo = parentHierarchy.children()
        self.assertEqual(len(parentChildrenUndo), 6)

        childPathsUndo = set([child.path() for child in parentChildrenUndo])
        self.assertTrue(newGroupPath not in childPathsUndo)
        self.assertTrue(ball5Path in childPathsUndo)
        self.assertTrue(ball3Path in childPathsUndo)

        if ufeUtils.ufeFeatureSetVersion() >= 3:
            cmds.redo()
        else:
            groupCmd.redo()

        # global selection should still have the group path.
        self.assertEqual(globalSelection.front(), ufe.Hierarchy.createItem(groupPath))

        parentChildrenRedo = parentHierarchy.children()
        self.assertEqual(len(parentChildrenRedo), 5)

        childPathsRedo = set([child.path() for child in parentChildrenRedo])
        self.assertTrue(newGroupPath in childPathsRedo)
        self.assertTrue(ball5Path not in childPathsRedo)
        self.assertTrue(ball3Path not in childPathsRedo)

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2022, 'Requires Maya fixes only available in Maya 2022 or greater.')
    def testGroupAcrossProxies(self):
        sphereFile = testUtils.getTestScene("groupCmd", "sphere.usda")
        sphereDagPath, sphereStage = mayaUtils.createProxyFromFile(sphereFile)
        usdSphere = sphereDagPath + ",/pSphere1"

        torusFile = testUtils.getTestScene("groupCmd", "torus.usda")
        torusDagPath, torusStage = mayaUtils.createProxyFromFile(torusFile)
        usdTorus = torusDagPath + ",/pTorus1"

        try:
            cmds.group(usdSphere, usdTorus)
        except Exception as e:
            self.assertTrue('cannot group across usd proxies')

    def testGroupKind(self):
        """
        Tests that grouping maintains a contiguous model hierarchy when the
        parent of the group is in the model hierarchy.
        """
        mayaPathSegment = mayaUtils.createUfePathSegment(
            "|transform1|proxyShape1")

        usdSegmentBall3 = usdUtils.createUfePathSegment(
            "/Ball_set/Props/Ball_3")
        ball3Path = ufe.Path([mayaPathSegment, usdSegmentBall3])
        ball3Item = ufe.Hierarchy.createItem(ball3Path)

        usdSegmentBall5 = usdUtils.createUfePathSegment(
            "/Ball_set/Props/Ball_5")
        ball5Path = ufe.Path([mayaPathSegment, usdSegmentBall5])
        ball5Item = ufe.Hierarchy.createItem(ball5Path)

        usdSegmentProps = usdUtils.createUfePathSegment("/Ball_set/Props")
        propsPath = ufe.Path([mayaPathSegment, usdSegmentProps])
        propsItem = ufe.Hierarchy.createItem(propsPath)

        ufeSelection = ufe.GlobalSelection.get()
        ufeSelection.append(ball3Item)
        ufeSelection.append(ball5Item)

        groupName = "newGroup"
        cmds.group(name=groupName)

        newGroupPath = propsPath + ufe.PathComponent("%s1" % groupName)
        newGroupItem = ufe.Hierarchy.createItem(newGroupPath)
        newGroupPrim = usdUtils.getPrimFromSceneItem(newGroupItem)

        # The "Props" prim that was the parent of both "Ball" prims has
        # kind=group and is part of the model hierarchy, so the new group prim
        # should also have kind=group and be included in the model hierarchy.
        self.assertEqual(Usd.ModelAPI(newGroupPrim).GetKind(), Kind.Tokens.group)
        self.assertTrue(newGroupPrim.IsModel())

        cmds.undo()

        # Clear the kind metadata on the "Props" prim before testing again.
        propsPrim = usdUtils.getPrimFromSceneItem(propsItem)
        Usd.ModelAPI(propsPrim).SetKind("")
        self.assertFalse(propsPrim.IsModel())

        # Freshen the UFE scene items so they have valid handles to their
        # UsdPrims.
        ball3Item = ufe.Hierarchy.createItem(ball3Path)
        ball5Item = ufe.Hierarchy.createItem(ball5Path)

        ufeSelection.clear()
        ufeSelection.append(ball3Item)
        ufeSelection.append(ball5Item)

        cmds.group(name=groupName)

        newGroupItem = ufe.Hierarchy.createItem(newGroupPath)
        newGroupPrim = usdUtils.getPrimFromSceneItem(newGroupItem)

        # When the "Props" prim does not have an authored kind and is not part
        # of the model hierarchy, the new group prim should not have any kind
        # authored either.
        self.assertEqual(Usd.ModelAPI(newGroupPrim).GetKind(), "")
        self.assertFalse(newGroupPrim.IsModel())

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2022, 'Grouping restriction is only available in Maya 2022 or greater.')
    def testGroupRestriction(self):
        ''' Verify group restriction. '''
        mayaPathSegment = mayaUtils.createUfePathSegment(
            "|transform1|proxyShape1")

        usdSegmentBall3 = usdUtils.createUfePathSegment(
            "/Ball_set/Props/Ball_3")
        ball3Path = ufe.Path([mayaPathSegment, usdSegmentBall3])
        ball3Item = ufe.Hierarchy.createItem(ball3Path)

        usdSegmentBall5 = usdUtils.createUfePathSegment(
            "/Ball_set/Props/Ball_5")
        ball5Path = ufe.Path([mayaPathSegment, usdSegmentBall5])
        ball5Item = ufe.Hierarchy.createItem(ball5Path)

        usdSegmentProps = usdUtils.createUfePathSegment("/Ball_set/Props")
        propsPath = ufe.Path([mayaPathSegment, usdSegmentProps])
        propsItem = ufe.Hierarchy.createItem(propsPath)

        ufeSelection = ufe.GlobalSelection.get()
        ufeSelection.append(ball3Item)
        ufeSelection.append(ball5Item)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # set the edit target to the session layer
        stage.SetEditTarget(stage.GetSessionLayer())

        # expect the exception happens
        with self.assertRaises(RuntimeError):
            cmds.group(name="newGroup")

        # set the edit target to the root layer
        stage.SetEditTarget(stage.GetRootLayer())

        # create a sphere
        stage.DefinePrim('/Sphere1', 'Sphere')

        # select the Sphere
        spherePath = ufe.PathString.path("{},/Sphere1".format("|transform1|proxyShape1"))
        sphereItem = ufe.Hierarchy.createItem(spherePath)
        ufeSelection = ufe.GlobalSelection.get()
        ufeSelection.clear()
        ufeSelection.append(sphereItem)

        # set the edit target to the session layer
        stage.SetEditTarget(stage.GetSessionLayer())

        # expect the exception happens.
        with self.assertRaises(RuntimeError):
            cmds.group(name="newGroup")

        # undo
        cmds.undo()

        # verify that group1 doesn't exist after the undo
        self.assertEqual([item for item in stage.Traverse()],
            [stage.GetPrimAtPath("/Ball_set"),
            stage.GetPrimAtPath("/Ball_set/Props"), 
            stage.GetPrimAtPath("/Ball_set/Props/Ball_1"),
            stage.GetPrimAtPath("/Ball_set/Props/Ball_2"),
            stage.GetPrimAtPath("/Ball_set/Props/Ball_3"),
            stage.GetPrimAtPath("/Ball_set/Props/Ball_4"),
            stage.GetPrimAtPath("/Ball_set/Props/Ball_5"),
            stage.GetPrimAtPath("/Ball_set/Props/Ball_6"),
            stage.GetPrimAtPath("/Sphere1")])

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 3, 'testGroupAbsolute is only available in UFE v3 or greater.')
    def testGroupAbsolute(self):
        '''Verify -absolute flag.'''

        cmds.file(new=True, force=True)

         # create a stage
        (stage, proxyShapePathStr, proxyShapeItem, contextOp) = createStage()

        # create a sphere generator
        sphereGen = SphereGenerator(1, contextOp, proxyShapePathStr)

        spherePath = sphereGen.createSphere()
        spherePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(spherePath))

        # no TRS attributes
        self.assertFalse(spherePrim.HasAttribute('xformOp:translate'))
        self.assertFalse(spherePrim.HasAttribute('xformOp:rotateXYZ'))
        self.assertFalse(spherePrim.HasAttribute('xformOp:scale'))

        # create a group with absolute flag set to True
        cmds.group(ufe.PathString.string(spherePath), absolute=True)

        # verify that groupItem has 1 child
        groupItem = ufe.GlobalSelection.get().front()
        groupHierarchy = ufe.Hierarchy.hierarchy(groupItem)
        self.assertEqual(len(groupHierarchy.children()), 1)

        # verify XformOpOrderAttr exist after grouping
        newspherePrim = stage.GetPrimAtPath("/group1/Sphere1")
        sphereXformable = UsdGeom.Xformable(newspherePrim)

        self.assertEqual(
            sphereXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray((
                "xformOp:translate", "xformOp:rotateXYZ", "xformOp:scale")))


    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 3, 'testGroupRelative is only available in UFE v3 or greater.')
    def testGroupRelative(self):
        '''Verify -relative flag.'''
        cmds.file(new=True, force=True)

         # create a stage
        (stage, proxyShapePathStr, proxyShapeItem, contextOp) = createStage()

        # create a sphere generator
        sphereGen = SphereGenerator(1, contextOp, proxyShapePathStr)

        spherePath = sphereGen.createSphere()
        spherePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(spherePath))

        # no TRS attributes
        self.assertFalse(spherePrim.HasAttribute('xformOp:translate'))
        self.assertFalse(spherePrim.HasAttribute('xformOp:rotateXYZ'))
        self.assertFalse(spherePrim.HasAttribute('xformOp:scale'))

        # create a group with relative flag set to True
        cmds.group(ufe.PathString.string(spherePath), relative=True)

        # verify that groupItem has 1 child
        groupItem = ufe.GlobalSelection.get().front()
        groupHierarchy = ufe.Hierarchy.hierarchy(groupItem)
        self.assertEqual(len(groupHierarchy.children()), 1)

        # verify XformOpOrderAttr exist after grouping
        newspherePrim = stage.GetPrimAtPath("/group1/Sphere1")

        # no TRS attributes
        self.assertFalse(newspherePrim.HasAttribute('xformOp:translate'))
        self.assertFalse(newspherePrim.HasAttribute('xformOp:rotateXYZ'))
        self.assertFalse(newspherePrim.HasAttribute('xformOp:scale'))

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 3, 'testGroupWorld is only available in UFE v3 or greater.')
    def testGroupWorld(self):
        '''Verify -world flag.'''
        cmds.file(new=True, force=True)

         # create a stage
        (stage, proxyShapePathStr, proxyShapeItem, contextOp) = createStage()

        # create a sphere generator
        sphereGen = SphereGenerator(3, contextOp, proxyShapePathStr)

        sphere1Path = sphereGen.createSphere()
        sphere1Prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(sphere1Path))

        sphere2Path = sphereGen.createSphere()
        sphere2Prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(sphere2Path))

        sphere3Path = sphereGen.createSphere()
        sphere3Prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(sphere3Path))

        # group Sphere1, Sphere2, and Sphere3
        groupName = cmds.group(ufe.PathString.string(sphere1Path),
                               ufe.PathString.string(sphere2Path),
                               ufe.PathString.string(sphere3Path))

        # verify that groupItem has 3 children
        groupItem = ufe.GlobalSelection.get().front()
        groupHierarchy = ufe.Hierarchy.hierarchy(groupItem)
        self.assertEqual(len(groupHierarchy.children()), 3)

        # group Sphere2 and Sphere3 with world flag enabled.
        # world flag puts the new group under the world
        cmds.group("{},/group1/Sphere2".format(proxyShapePathStr), 
                   "{},/group1/Sphere3".format(proxyShapePathStr), world=True)

        # verify group2 was created under the proxyshape
        self.assertEqual([item for item in stage.Traverse()],
            [stage.GetPrimAtPath("/group1"),
            stage.GetPrimAtPath("/group1/Sphere1"), 
            stage.GetPrimAtPath("/group2"),
            stage.GetPrimAtPath("/group2/Sphere2"),
            stage.GetPrimAtPath("/group2/Sphere3")])

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 3, 'testGroupUndoRedo is only available in UFE v3 or greater.')
    def testGroupUndoRedo(self):
        '''Verify grouping after multiple undo/redo.'''
        cmds.file(new=True, force=True)

         # create a stage
        (stage, proxyShapePathStr, proxyShapeItem, contextOp) = createStage()

        # create a sphere generator
        sphereGen = SphereGenerator(3, contextOp, proxyShapePathStr)

        sphere1Path = sphereGen.createSphere()
        sphere1Prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(sphere1Path))

        sphere2Path = sphereGen.createSphere()
        sphere2Prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(sphere2Path))

        sphere3Path = sphereGen.createSphere()
        sphere3Prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(sphere3Path))

        # group Sphere1, Sphere2, and Sphere3
        groupName = cmds.group(ufe.PathString.string(sphere1Path),
                               ufe.PathString.string(sphere2Path),
                               ufe.PathString.string(sphere3Path))

        # verify that groupItem has 3 children
        groupItem = ufe.GlobalSelection.get().front()
        groupHierarchy = ufe.Hierarchy.hierarchy(groupItem)
        self.assertEqual(len(groupHierarchy.children()), 3)

        cmds.undo()

        self.assertEqual([item for item in stage.Traverse()],
            [stage.GetPrimAtPath("/Sphere3"), 
            stage.GetPrimAtPath("/Sphere2"),
            stage.GetPrimAtPath("/Sphere1")])

        cmds.redo()

        self.assertEqual([item for item in stage.Traverse()],
            [stage.GetPrimAtPath("/group1"),
            stage.GetPrimAtPath("/group1/Sphere1"), 
            stage.GetPrimAtPath("/group1/Sphere2"),
            stage.GetPrimAtPath("/group1/Sphere3")])

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 3, 'testGroupUndoRedo is only available in UFE v3 or greater.')
    def testGroupPreserveLoadRules(self):
        '''Verify load rules are preserved when grouping.'''
        cmds.file(new=True, force=True)

         # create a stage
        (stage, proxyShapePathStr, proxyShapeItem, contextOp) = createStage()

        # create a sphere generator
        sphereGen = SphereGenerator(3, contextOp, proxyShapePathStr)

        sphere1Path = sphereGen.createSphere()
        sphere1Prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(sphere1Path))

        sphere2Path = sphereGen.createSphere()
        sphere2Prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(sphere2Path))

        sphere3Path = sphereGen.createSphere()
        sphere3Prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(sphere3Path))

        # Setup the load rules:
        #     /Sphere1 is unloaded
        #     /Sphere2 is loaded
        loadRules = stage.GetLoadRules()
        loadRules.AddRule('/Sphere1', loadRules.NoneRule)
        loadRules.AddRule('/Sphere2', loadRules.AllRule)
        stage.SetLoadRules(loadRules)

        self.assertEqual(loadRules.NoneRule, stage.GetLoadRules().GetEffectiveRuleForPath('/Sphere1'))
        self.assertEqual(loadRules.AllRule, stage.GetLoadRules().GetEffectiveRuleForPath('/Sphere2'))

        # group Sphere1, Sphere2, and Sphere3
        groupName = cmds.group(ufe.PathString.string(sphere1Path),
                               ufe.PathString.string(sphere2Path),
                               ufe.PathString.string(sphere3Path))

        # verify that groupItem has 3 children
        groupItem = ufe.GlobalSelection.get().front()
        groupHierarchy = ufe.Hierarchy.hierarchy(groupItem)
        self.assertEqual(len(groupHierarchy.children()), 3)

        self.assertEqual(loadRules.NoneRule, stage.GetLoadRules().GetEffectiveRuleForPath('/group1/Sphere1'))
        self.assertEqual(loadRules.AllRule, stage.GetLoadRules().GetEffectiveRuleForPath('/group1/Sphere2'))

        cmds.undo()

        self.assertEqual(loadRules.NoneRule, stage.GetLoadRules().GetEffectiveRuleForPath('/Sphere1'))
        self.assertEqual(loadRules.AllRule, stage.GetLoadRules().GetEffectiveRuleForPath('/Sphere2'))

        cmds.redo()

        self.assertEqual(loadRules.NoneRule, stage.GetLoadRules().GetEffectiveRuleForPath('/group1/Sphere1'))
        self.assertEqual(loadRules.AllRule, stage.GetLoadRules().GetEffectiveRuleForPath('/group1/Sphere2'))

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Requires Maya fixes only available in Maya 2023 or greater.')
    def testGroupHierarchy(self):
        '''Grouping a node and a descendant.'''
        # MAYA-112957: when grouping a node and its descendant, with the node
        # selected first, the descendant path becomes stale as soon as its
        # ancestor gets reparented.  The Maya parent command must deal with
        # this.  A similar test is done for parenting in testParentCmd.py

        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer

        # Create the following hierarchy:
        #
        # ps
        #  |_ A
        #      |_ B
        #          |_ C
        #  |_ D
        #      |_ E
        #          |_ F
        #
        #  |_ G
        #
        # We will select A, B, C, E, F and G, in order, and group.  This will
        # create a new group under ps, with A, B, C, E, F and G as children.

        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/A', 'Xform')
        stage.DefinePrim('/A/B', 'Xform')
        stage.DefinePrim('/A/B/C', 'Xform')
        stage.DefinePrim('/D', 'Xform')
        stage.DefinePrim('/D/E', 'Xform')
        stage.DefinePrim('/D/E/F', 'Xform')
        stage.DefinePrim('/G', 'Xform')

        psPath = ufe.PathString.path(psPathStr)
        psPathSegment = psPath.segments[0]
        ps = ufe.Hierarchy.createItem(psPath)
        psHier = ufe.Hierarchy.hierarchy(ps)
        dPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/D')])
        d = ufe.Hierarchy.createItem(dPath)
        dHier = ufe.Hierarchy.hierarchy(d)
        groupPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/group1')])

        def hierarchyBefore():
            aPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/A')])
            a = ufe.Hierarchy.createItem(aPath)
            bPath = aPath + ufe.PathComponent('B')
            b = ufe.Hierarchy.createItem(bPath)
            cPath = bPath + ufe.PathComponent('C')
            c = ufe.Hierarchy.createItem(cPath)
            ePath = dPath + ufe.PathComponent('E')
            e = ufe.Hierarchy.createItem(ePath)
            fPath = ePath + ufe.PathComponent('F')
            f = ufe.Hierarchy.createItem(fPath)
            gPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/G')])
            g = ufe.Hierarchy.createItem(gPath)
            return [a, b, c, e, f, g]

        def hierarchyAfter():
            return [ufe.Hierarchy.createItem(groupPath + ufe.PathComponent(pc)) for pc in ['A', 'B', 'C', 'E', 'F', 'G']]

        def checkBefore(a, b, c, e, f, g):
            psChildren = psHier.children()
            aHier = ufe.Hierarchy.hierarchy(a)
            bHier = ufe.Hierarchy.hierarchy(b)
            cHier = ufe.Hierarchy.hierarchy(c)
            eHier = ufe.Hierarchy.hierarchy(e)
            fHier = ufe.Hierarchy.hierarchy(f)
            gHier = ufe.Hierarchy.hierarchy(g)

            self.assertIn(a, psChildren)
            self.assertIn(d, psChildren)
            self.assertIn(g, psChildren)
            self.assertIn(b, aHier.children())
            self.assertIn(c, bHier.children())
            self.assertIn(e, dHier.children())
            self.assertIn(f, eHier.children())
            self.assertFalse(gHier.hasChildren())

        def checkAfter(a, b, c, e, f, g):
            psChildren = psHier.children()
            self.assertNotIn(a, psChildren)
            self.assertIn(d, psChildren)
            self.assertNotIn(g, psChildren)

            group = ufe.Hierarchy.createItem(groupPath)
            groupHier = ufe.Hierarchy.hierarchy(group)
            groupChildren = groupHier.children()

            for child in [a, b, c, e, f, g]:
                hier = ufe.Hierarchy.hierarchy(child)
                self.assertFalse(hier.hasChildren())
                self.assertIn(child, groupChildren)

        children = hierarchyBefore()
        checkBefore(*children)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        for child in children:
            sn.append(child)

        cmds.group()

        children = hierarchyAfter()
        checkAfter(*children)

        cmds.undo()

        children = hierarchyBefore()
        checkBefore(*children)

        cmds.redo()

        children = hierarchyAfter()
        checkAfter(*children)

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Requires Maya fixes only available in Maya 2023 or greater.')
    def testGroupHierarchyWithRenaming(self):
        '''Grouping a node and a descendant when all descendants share the same name'''
        # MAYA-113532: when grouping a node and its descendant sharing the same name, we need to
        # detect the name conflicts and rename as we reparent into the group.

        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer

        # Create the following hierarchy:
        #
        # ps
        #  |_ A
        #      |_ A
        #          |_ A
        #              |_ A
        #                  |_ A
        #
        # And group them all at the same level, which implies renaming.

        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/A', 'Xform')
        stage.DefinePrim('/A/B', 'Cube')
        stage.DefinePrim('/A/A', 'Xform')
        stage.DefinePrim('/A/A/C', 'Cone')
        stage.DefinePrim('/A/A/A', 'Xform')
        stage.DefinePrim('/A/A/A/D', 'Sphere')
        stage.DefinePrim('/A/A/A/A', 'Xform')
        stage.DefinePrim('/A/A/A/A/E', 'Capsule')
        stage.DefinePrim('/A/A/A/A/A', 'Xform')
        stage.DefinePrim('/A/A/A/A/A/F', 'Cylinder')

        psPath = ufe.PathString.path(psPathStr)
        psPathSegment = psPath.segments[0]
        ps = ufe.Hierarchy.createItem(psPath)
        psHier = ufe.Hierarchy.hierarchy(ps)
        groupPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/group1')])

        def hierarchyBefore():
            retVal = []
            aPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/A')])
            retVal.append(ufe.Hierarchy.createItem(aPath))
            for i in range(4):
                aPath = aPath + ufe.PathComponent('A')
                retVal.append(ufe.Hierarchy.createItem(aPath))
            return retVal

        a, aa, aaa, aaaa, aaaaa = hierarchyBefore()

        def checkBefore():
            # We care about 2 things:
            #  - All Xforms are named A
            #  - The non-xform stay in the right order:
            order = [("B", "Cube"), ("C", "Cone"), ("D", "Sphere"), ("E", "Capsule"), ("F", "Cylinder")]
            psChildren = psHier.children()
            self.assertEqual(len(psChildren), 1)
            a = psChildren[0]
            for gprimName, gprimTypeName in order:
                aHier = ufe.Hierarchy.hierarchy(a)
                aChildren = aHier.children()
                if gprimName == "F":
                    self.assertEqual(len(aChildren), 1)
                else:
                    self.assertEqual(len(aChildren), 2)
                for child in aChildren:
                    prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(child.path()))
                    if child.nodeName() == "A":
                        self.assertEqual(prim.GetTypeName(), "Xform")
                        a = child
                    else:
                        self.assertEqual(child.nodeName(), gprimName)
                        self.assertEqual(prim.GetTypeName(), gprimTypeName)

        def checkAfter():
            # We care about 2 things:
            #  - Group has 5 Xform children. Don't care about names: they will be unique.
            #  - Each child has a one of the non-xform prims.
            members = set([("B", "Cube"), ("C", "Cone"), ("D", "Sphere"), ("E", "Capsule"), ("F", "Cylinder")])
            group = ufe.Hierarchy.createItem(groupPath)
            groupHier = ufe.Hierarchy.hierarchy(group)
            groupChildren = groupHier.children()
            self.assertEqual(len(groupChildren), 5)
            foundMembers = set()
            for child in groupChildren:
                prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(child.path()))
                self.assertEqual(prim.GetTypeName(), "Xform")
                childHier = ufe.Hierarchy.hierarchy(child)
                childChildren = childHier.children()
                self.assertEqual(len(childChildren), 1)
                member = childChildren[0]
                prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(member.path()))
                foundMembers.add((member.nodeName(), prim.GetTypeName()))
            self.assertEqual(members, foundMembers)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        # We randomize the order a bit to make sure previously moved items do
        # not affect the next one.
        sn.append(a)
        sn.append(aaa)
        sn.append(aa)
        sn.append(aaaaa)
        sn.append(aaaa)

        cmds.group()
        checkAfter()
        cmds.undo()
        checkBefore()
        cmds.redo()
        checkAfter()

if __name__ == '__main__':
    unittest.main(verbosity=2)
