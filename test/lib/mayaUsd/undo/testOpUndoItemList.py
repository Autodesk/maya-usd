#!/usr/bin/env python

#
# Copyright 2023 Autodesk
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

from usdUtils import createSimpleXformScene

import mayaUsd.lib

import mayaUtils

from maya import cmds
from maya import standalone

import unittest

class OpUndoItemListTestCase(unittest.TestCase):
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
        cmds.file(new=True, force=True)

    def testUndoItemList(self):
        '''
        Test that undo items capture in a OpUndoItemList can
        be undone and redone.
        '''

        (_, _, aXlation, aUsdUfePathStr, aUsdUfePath, _, _, 
               bXlation, bUsdUfePathStr, bUsdUfePath, _) = \
            createSimpleXformScene()

        # Verify the item list is initially empty.
        undoList = mayaUsd.lib.OpUndoItemList()
        self.assertTrue(undoList.isEmpty())

        # Duplicate USD data as Maya data, placing it under the root.
        with undoList:
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.duplicate(
                aUsdUfePathStr, ''))

        # Verify the command worked and undo items were captured.
        def hasDuplicatedObjects():
            # Should now have two transform nodes in the Maya scene: the path
            # components in the second segment of the aUsdItem and bUsdItem will
            # now be under the Maya world.
            aMayaPathStr = str(aUsdUfePath.segments[1]).replace('/', '|')
            bMayaPathStr = str(bUsdUfePath.segments[1]).replace('/', '|')
            aMayaNodes = cmds.ls(aMayaPathStr, long=True)
            bMayaNodes = cmds.ls(bMayaPathStr, long=True)
            return len(aMayaNodes) > 0 and aMayaNodes[0] == aMayaPathStr and \
                   len(bMayaNodes) > 0 and bMayaNodes[0] == bMayaPathStr
        
        self.assertFalse(undoList.isEmpty())
        self.assertTrue(hasDuplicatedObjects())

        # Verify that undoing the item list removes the Maya objects
        # but the item list stays valid.
        undoList.undo()

        self.assertFalse(undoList.isEmpty())
        self.assertFalse(hasDuplicatedObjects())

        # Verify that redoing the item list restores the Maya objects
        # but the item list stays valid.
        undoList.redo()
        
        self.assertFalse(undoList.isEmpty())
        self.assertTrue(hasDuplicatedObjects())

        # Clear the list and verify that it is empty.
        undoList.clear()
        self.assertTrue(undoList.isEmpty())

        # Verify that undo and redo do nothing with the empty list.
        undoList.undo()
        self.assertTrue(undoList.isEmpty())
        self.assertTrue(hasDuplicatedObjects())

        undoList.redo()
        self.assertTrue(undoList.isEmpty())
        self.assertTrue(hasDuplicatedObjects())


if __name__ == '__main__':
    unittest.main(verbosity=2)
