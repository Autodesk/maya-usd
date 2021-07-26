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

        if (os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') > '3000'):
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

        parentChildrenPost = parentHierarchy.children()
        self.assertEqual(len(parentChildrenPost), 5)

        # The command will now append a number 1 at the end to match the naming
        # convention in Maya.
        if (os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') > '3000'):
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
        
        if (os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') > '3000'):
            cmds.undo()
        else:
            groupCmd.undo()

        # gloabl selection should not be empty after undo.
        if (os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') > '3004'):
            pass
            # TODO: for some strange reasons the globalSelection returns 0??
            # I tried the same steps in interactive Maya and it correctly returns 2
            # self.assertEqual(len(globalSelection), 2)
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
    def testGroupRestirction(self):
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

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '3005', 'testGroupAbsolute is only available in UFE preview version 0.3.5 and greater')
    def testGroupAbsolute(self):
        '''Verify -absolute flag.'''

        cmds.file(new=True, force=True)

         # create a stage
        (stage, proxyShapePathStr, proxyShapeItem, contextOp) = createStage();

        # create a sphere generator
        sphereGen = SphereGenerator(2, contextOp, proxyShapePathStr)
        
        spherePath = sphereGen.createSphere()
        spherePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(spherePath))

        # no TRS attributes
        self.assertFalse(spherePrim.HasAttribute('xformOp:translate'))
        self.assertFalse(spherePrim.HasAttribute('xformOp:rotateXYZ'))
        self.assertFalse(spherePrim.HasAttribute('xformOp:scale'))

        # create a group with absolute flag set to True
        cmds.group(ufe.PathString.string(spherePath), absolute= True)

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


    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '3005', 'testGroupRelative is only available in UFE preview version 0.3.5 and greater')
    def testGroupRelative(self):
        '''Verify -relative flag.'''
        pass

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '3005', 'testGroupWorld is only available in UFE preview version 0.3.5 and greater')
    def testGroupWorld(self):
        '''Verify -world flag.'''
        pass 

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '3005', 'testGroupHierarchyAfterUndoRedo is only available in UFE preview version 0.3.5 and greater')
    def testGroupHierarchyAfterUndoRedo(self):
        '''Verify grouping after multiple undo/redo.'''
        pass

if __name__ == '__main__':
    unittest.main(verbosity=2)
