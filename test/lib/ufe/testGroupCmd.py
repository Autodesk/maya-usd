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

import os

import maya.cmds as cmds

from pxr import Sdf

from ufeTestUtils import usdUtils, mayaUtils, ufeUtils
import ufe

import unittest

class GroupCmdTestCase(unittest.TestCase):
    '''Verify the Maya group command, for multiple runtimes.

    As of 19-Nov-2018, the UFE group command is not integrated into Maya, so
    directly test the UFE undoable command.
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
        
        # Open top_layer.ma scene in test-samples
        mayaUtils.openTopLayerScene()
        
        # Clear selection to start off
        cmds.select(clear=True)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2017', 'testUsdGroup did not really test anything before UFE 2.17')
    def testUsdGroup(self):
        '''Creation of USD group objects.'''

        ball25Path = ufe.Path([
            mayaUtils.createUfePathSegment("|world|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_25")])
        ball25Item = ufe.Hierarchy.createItem(ball25Path)

        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|world|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)

        ufeSelectionList = ufe.Selection()
        ufeSelectionList.append(ball25Item)
        ufeSelectionList.append(ball35Item)

        parentPath = ufe.Path([
            mayaUtils.createUfePathSegment("|world|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props")])
        parentItem = ufe.Hierarchy.createItem(parentPath)

        parentHierarchy = ufe.Hierarchy.hierarchy(parentItem)
        
        parentChildrenPre = parentHierarchy.children()
        self.assertEqual(len(parentChildrenPre), 35)

        newGroupName = ufe.PathComponent("newGroup")

        groupCmd = parentHierarchy.createGroupCmd(ufeSelectionList, newGroupName)
        groupCmd.execute()

        parentChildrenPost = parentHierarchy.children()
        self.assertEqual(len(parentChildrenPost), 34)

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
        self.assertTrue(ball25Path not in childPaths)
        self.assertTrue(ball35Path not in childPaths)

        groupCmd.undo()

        parentChildrenUndo = parentHierarchy.children()
        self.assertEqual(len(parentChildrenUndo), 35)

        childPathsUndo = set([child.path() for child in parentChildrenUndo])
        self.assertTrue(newGroupPath not in childPathsUndo)
        self.assertTrue(ball25Path in childPathsUndo)
        self.assertTrue(ball35Path in childPathsUndo)

        groupCmd.redo()

        parentChildrenRedo = parentHierarchy.children()
        self.assertEqual(len(parentChildrenRedo), 34)

        childPathsRedo = set([child.path() for child in parentChildrenRedo])
        self.assertTrue(newGroupPath in childPathsRedo)
        self.assertTrue(ball25Path not in childPathsRedo)
        self.assertTrue(ball35Path not in childPathsRedo)

