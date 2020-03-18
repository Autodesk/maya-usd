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

from pxr import Sdf

from ufeTestUtils import usdUtils, mayaUtils
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

    def testUsdGroup(self):
        '''Creation of USD group objects.'''

        # Get parent of new group.
        propsPath = ufe.Path([
            mayaUtils.createUfePathSegment(
                "|world|transform1|proxyShape1"),
             usdUtils.createUfePathSegment("/Room_set/Props")])
        propsItem = ufe.Hierarchy.createItem(propsPath)
        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildrenPre = propsHierarchy.children()

        groupPath = propsPath + "newGroup"

        # Create new group.
        group = propsHierarchy.createGroupCmd(ufe.PathComponent("newGroup"))

        self.assertIsNotNone(group.item)
        # MAYA-92350: must re-create hierarchy interface object.  Fix ASAP.
        # PPT, 19-Nov-2018.
        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildrenPost = propsHierarchy.children()
        self.assertEqual(len(propsChildrenPre)+1, len(propsChildrenPost))
        childrenPaths = set([child.path() for child in propsChildrenPost])
        self.assertTrue(groupPath in childrenPaths)

        # Undo
        group.undoableCommand.undo()

        # MAYA-92350: must re-create hierarchy interface object.  Fix ASAP.
        # PPT, 19-Nov-2018.
        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildrenPostUndo = propsHierarchy.children()
        self.assertEqual(len(propsChildrenPre), len(propsChildrenPostUndo))
        childrenPaths = set([child.path() for child in propsChildrenPostUndo])
        self.assertFalse(groupPath in childrenPaths)

        # MAYA-92264: redo doesn't work.
        # group.undoableCommand.redo()
