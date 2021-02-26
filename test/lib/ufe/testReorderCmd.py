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

import fixturesUtils
import mayaUtils
import usdUtils

import mayaUsd.ufe

from maya import cmds
from maya import standalone

import ufe

import os
import re
import unittest


def findIndex(childItem):
    hier = ufe.Hierarchy.hierarchy(childItem)
    childrenList = ufe.Hierarchy.hierarchy(hier.parent()).children();
    index = childrenList.index(childItem)
    return index

class ReorderCmdTestCase(unittest.TestCase):
    '''Verify the Maya reorder command on a USD scene.'''

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
        # load plugins
        self.assertTrue(self.pluginsLoaded)

        # open primitives.ma scene in test-samples
        mayaUtils.openPrimitivesScene()

        # clear selection to start off
        cmds.select(clear=True)

    def testReorderRelative(self):
        ''' Reorder (a.k.a move) objects relative to their siblings.
           For relative moves, both positive and negative numbers may be specified.
        '''
        cylinderPath = ufe.PathString.path("|Primitives_usd|Primitives_usdShape,/Xform1/Cylinder1")
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        # start by getting the children from the parent node ( Xform1 )
        hier = ufe.Hierarchy.hierarchy(cylinderItem)
        childrenList = ufe.Hierarchy.hierarchy(hier.parent()).children();

        # get Cylinder1 position ( index ) before the move
        index = childrenList.index(cylinderItem)
        self.assertEqual(index, 3)

        # move Cylinder1 forward ( + ) by 3
        cmds.reorder("|Primitives_usd|Primitives_usdShape,/Xform1/Cylinder1", r=3)

        # check Cylinder1 position ( index ) after the move
        self.assertEqual(findIndex(cylinderItem), 6)

        # move Cylinder1 backward ( - ) by 3
        cmds.reorder("|Primitives_usd|Primitives_usdShape,/Xform1/Cylinder1", r=-3)

        # check Cylinder1 position ( index ) after the move
        self.assertEqual(findIndex(cylinderItem), 3)

    def testFrontBackRelative(self):
        '''
            Reorder (a.k.a move) to front and back of the sibling list
        '''
        cylinderPath = ufe.PathString.path("|Primitives_usd|Primitives_usdShape,/Xform1/Cylinder1")
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        # make Cylinder1 the first sibling ( front )
        cmds.reorder(ufe.PathString.string(cylinderPath), front=True)

        # check Cylinder1 position ( index ) after the move
        self.assertEqual(findIndex(cylinderItem), 0)

        # make Cylinder1 the last sibling ( back )
        cmds.reorder(ufe.PathString.string(cylinderPath), back=True)

        # check Cylinder1 position ( index ) after the move
        self.assertEqual(findIndex(cylinderItem), 7) 

    def testUndoRedo(self):
        ''' 
            Undo/Redo reorder command
        '''
        # create multiple item
        capsulePath = ufe.PathString.path("|Primitives_usd|Primitives_usdShape,/Xform1/Capsule1")
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        
        conePath = ufe.PathString.path("|Primitives_usd|Primitives_usdShape,/Xform1/Cone1")
        coneItem = ufe.Hierarchy.createItem(conePath)
        
        cubePath = ufe.PathString.path("|Primitives_usd|Primitives_usdShape,/Xform1/Cube1")
        cubeItem = ufe.Hierarchy.createItem(cubePath)

        # move items forward ( + )
        cmds.reorder(ufe.PathString.string(capsulePath), r=3)
        cmds.reorder(ufe.PathString.string(conePath),    r=3)
        cmds.reorder(ufe.PathString.string(cubePath),    r=3)

        # check positions
        self.assertEqual(findIndex(capsuleItem), 1)
        self.assertEqual(findIndex(coneItem),    2) 
        self.assertEqual(findIndex(cubeItem),    3)

        # undo
        for _ in range(3):
            cmds.undo()

        # check positions
        self.assertEqual(findIndex(capsuleItem), 0)
        self.assertEqual(findIndex(coneItem),    1) 
        self.assertEqual(findIndex(cubeItem),    2)

        # redo
        for _ in range(3):
            cmds.redo()

        # check positions
        self.assertEqual(findIndex(capsuleItem), 1)
        self.assertEqual(findIndex(coneItem),    2) 
        self.assertEqual(findIndex(cubeItem),    3)


if __name__ == '__main__':
    unittest.main(verbosity=2)
