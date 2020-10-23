#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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

import usdUtils, mayaUtils

import ufe
import mayaUsd.ufe
import maya.cmds as cmds

import unittest
import re
import os

class ReorderCmdTestCase(unittest.TestCase):
    '''Verify the Maya parent command on a USD scene.'''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    @classmethod
    def tearDownClass(cls):
        cmds.file(new=True, force=True)

    def setUp(self):
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # Open primitives.ma scene in test-samples
        mayaUtils.openPrimitivesScene()

        # Clear selection to start off
        cmds.select(clear=True)

    def testReorderRelative(self):
        ''' Reorder (a.k.a move) objects relative to their siblings.
           For relative moves, both positive and negative numbers may be specified.
        '''
        shapeSegment = mayaUtils.createUfePathSegment("|world|Primitives_usd|Primitives_usdShape")
        cylinderPath = ufe.Path([shapeSegment, usdUtils.createUfePathSegment("/Xform1/Cylinder1")])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        # Start by getting the children form the parent node ( Xform1 )
        hier = ufe.Hierarchy.hierarchy(cylinderItem)
        childrenList = ufe.Hierarchy.hierarchy(hier.parent()).children();

        # Get Cylinder1 position ( index ) before the move
        index = childrenList.index(cylinderItem)
        self.assertEqual(index, 3)

        # move Cylinder1 forward ( + ) 3 siblings.
        cmds.reorder("|Primitives_usd|Primitives_usdShape,/Xform1/Cylinder1", r=3)

        # check Cylinder1 position ( index ) after the move
        hier = ufe.Hierarchy.hierarchy(cylinderItem)
        childrenList = ufe.Hierarchy.hierarchy(hier.parent()).children();
        index = childrenList.index(cylinderItem)
        self.assertEqual(index, 6)

        # Undo
        cmds.undo()

        # check Cylinder1 position ( index ) after the Undo
        hier = ufe.Hierarchy.hierarchy(cylinderItem)
        childrenList = ufe.Hierarchy.hierarchy(hier.parent()).children();
        index = childrenList.index(cylinderItem)
        self.assertEqual(index, 3)

        # move Cylinder1 backward ( - ) 3 siblings.
        cmds.reorder("|Primitives_usd|Primitives_usdShape,/Xform1/Cylinder1", r=-3)

        # check Cylinder1 position ( index ) after the move
        hier = ufe.Hierarchy.hierarchy(cylinderItem)
        childrenList = ufe.Hierarchy.hierarchy(hier.parent()).children();
        index = childrenList.index(cylinderItem)
        self.assertEqual(index, 0)
