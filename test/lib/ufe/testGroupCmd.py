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

from pxr import Kind
from pxr import Usd

from maya import cmds
from maya import standalone

import ufe

import os
import unittest


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

        newGroupName = ufe.PathComponent("newGroup")

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # set the edit target to balls.usda
        layer = stage.GetLayerStack()[1]
        self.assertEqual("ballset.usda", layer.GetDisplayName())
        stage.SetEditTarget(layer)

        ufeSelectionList = ufe.Selection()
        ufeSelectionList.append(ball5Item)
        ufeSelectionList.append(ball3Item)

        groupCmd = parentHierarchy.createGroupCmd(
            ufeSelectionList, newGroupName)
        groupCmd.execute()

        parentChildrenPost = parentHierarchy.children()
        self.assertEqual(len(parentChildrenPost), 5)

        # The command will now append a number 1 at the end to match the naming
        # convention in Maya.
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

        groupCmd.undo()

        parentChildrenUndo = parentHierarchy.children()
        self.assertEqual(len(parentChildrenUndo), 6)

        childPathsUndo = set([child.path() for child in parentChildrenUndo])
        self.assertTrue(newGroupPath not in childPathsUndo)
        self.assertTrue(ball5Path in childPathsUndo)
        self.assertTrue(ball3Path in childPathsUndo)

        groupCmd.redo()

        parentChildrenRedo = parentHierarchy.children()
        self.assertEqual(len(parentChildrenRedo), 5)

        childPathsRedo = set([child.path() for child in parentChildrenRedo])
        self.assertTrue(newGroupPath in childPathsRedo)
        self.assertTrue(ball5Path not in childPathsRedo)
        self.assertTrue(ball3Path not in childPathsRedo)

    @unittest.skipUnless(mayaUtils.previewReleaseVersion() >= 121, 'Requires Maya fixes only available in Maya Preview Release 121 or later.')
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


if __name__ == '__main__':
    unittest.main(verbosity=2)
