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
import unittest

import maya.cmds as cmds

from pxr import Tf, Usd, UsdGeom, Gf

import mayaUsd.lib as mayaUsdLib

class TestUsdUndoManager(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cmds.loadPlugin('mayaUsdPlugin')

    def setUp(self):
        # create a stage in memory
        self.stage = Usd.Stage.CreateInMemory()

        # track the edit target layer
        mayaUsdLib.UsdUndoManager.trackLayerStates(self.stage.GetRootLayer())

        # clear selection to start off
        cmds.select(clear=True)

    def testSimpleUndoRedo(self):
        '''
            Simple test to demonstrate the basic usage of undo/redo service.
        '''
        # start with a new file
        cmds.file(force=True, new=True)

        # get pseudo root prim
        defaultPrim = self.stage.GetPseudoRoot()

        # get the current number of commands on the undo queue
        nbCmds = cmds.undoInfo(q=True)
        self.assertEqual(cmds.undoInfo(q=True), 0)

        self.assertEqual(len(defaultPrim.GetChildren()), 0)

        # create undo block
        with mayaUsdLib.UsdUndoBlock():
            prim = self.stage.DefinePrim('/World', 'Sphere')
            self.assertTrue(bool(prim))

        # check number of children under the root
        self.assertEqual(len(defaultPrim.GetChildren()), 1)

        # demonstrate there is one additional command on the undo queue
        # and that it's our command.
        self.assertEqual(cmds.undoInfo(q=True), nbCmds+1)

        # undo
        cmds.undo()

        # check number of children under the root
        self.assertEqual(len(defaultPrim.GetChildren()), 0)

        # redo
        cmds.redo()

        # check number of children under the root
        self.assertEqual(len(defaultPrim.GetChildren()), 1)

    def testNestedUsdUndoBlock(self):
        '''
            Nested UsdUndoBlock are supported but only the top level block
            will transfer the edits to UsdUndoableItem.
        '''
        # start with a new file
        cmds.file(force=True, new=True)

        # get the current number of commands on the undo queue
        nbCmds = cmds.undoInfo(q=True)
        self.assertEqual(cmds.undoInfo(q=True), 0)

        with mayaUsdLib.UsdUndoBlock():
            prim = self.stage.DefinePrim('/World')

        with mayaUsdLib.UsdUndoBlock():
            prim.SetActive(False)
            with mayaUsdLib.UsdUndoBlock():
                prim.SetActive(True)
                with mayaUsdLib.UsdUndoBlock():
                    prim.SetActive(False)

        # expect to have 2 items on the undo queue
        self.assertEqual(cmds.undoInfo(q=True), nbCmds+2)
