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
import pxr.Tf

class OpUndoItemTestCase(unittest.TestCase):
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

    def testUndoItemWithoutRecorder(self):
        '''
        Test that creating an undo item without a recorder raises errors in non-release builds.
        '''

        (_, _, aXlation, aUsdUfePathStr, aUsdUfePath, _, _, 
               bXlation, bUsdUfePathStr, bUsdUfePath, _) = \
            createSimpleXformScene()

        # Duplicate USD data as Maya data, placing it under the root
        # Without any means to capture undo items.
        with self.assertRaises(pxr.Tf.ErrorException):
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.duplicate(
                aUsdUfePathStr, '|world'))

    def testUndoItemWithRecorder(self):
        '''
        Test that creating an undo item with a recorder succeeds even in non-release builds
        that validate the environment undo items are created in.

        Note that in Python, muting is achieved by using an undo item list and throwing it
        away without preserving it in a long-lived object. So we don't need to explicitly
        test that.
        '''

        (_, _, aXlation, aUsdUfePathStr, aUsdUfePath, _, _, 
               bXlation, bUsdUfePathStr, bUsdUfePath, _) = \
            createSimpleXformScene()

        # Verify the item list is initially empty.
        undoList = mayaUsd.lib.OpUndoItemList()
        self.assertTrue(undoList.isEmpty())

        # Duplicate USD data as Maya data, placing it under the root
        # With an active list the undo item should be happy.
        with undoList:
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.duplicate(
                aUsdUfePathStr, '|world'))


if __name__ == '__main__':
    unittest.main(verbosity=2)
