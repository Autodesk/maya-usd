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

class UngroupCmdTestCase(unittest.TestCase):

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

    def testUngroupCommand(self):
        '''Simple test for ungroup command.'''

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

        # Group object (a.k.a parent) will be added to selection list. This behavior matches the native Maya group command.
        globalSelection = ufe.GlobalSelection.get()

        groupPath = ufe.Path([mayaPathSegment, usdUtils.createUfePathSegment("/Ball_set/Props/newGroup1")])
        self.assertEqual(globalSelection.front(), ufe.Hierarchy.createItem(groupPath))

        # check parentHierarchy children size
        self.assertEqual(len(parentHierarchy.children()), 5)

        # ungroup
        groupHierarchy = ufe.Hierarchy.hierarchy(globalSelection.front())
        ungroupCmd = groupHierarchy.ungroupCmd()
        ungroupCmd.execute()

        # check parentHierarchy children size
        self.assertEqual(len(parentHierarchy.children()), 6)

        ungroupCmd.undo()

        # check parentHierarchy children size
        self.assertEqual(len(parentHierarchy.children()), 5)

        ungroupCmd.redo()

        self.assertEqual(len(parentHierarchy.children()), 6)

if __name__ == '__main__':
    unittest.main(verbosity=2)
