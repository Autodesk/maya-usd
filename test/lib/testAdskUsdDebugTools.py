#!/usr/bin/env mayapy
#
# Copyright 2026 Autodesk
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

from pxr import Sdf, Usd

import mayaUsd.lib as mayaUsdLib


class testAdskUsdDebugTools(unittest.TestCase):
    """
    Verify that the Autodesk USD Debug Tools component is installed and loadable.
    """

    def testDebugToolsLoaded(self):
        try:
            # Import the USD Python bindings first so the boost.python to/from-Python
            # converters for USD types (e.g. SdfPath) are registered.
            from pxr import Usd, Sdf
            import AdskUsdDebug
        except Exception as e:
            self.fail(f"Autodesk USD Debug Tools module not available or failed to load. {e}")


class testAdskUsdDebugToolsRemoveOpinionUndo(unittest.TestCase):
    """
    Verify that removing an opinion through the Debug Tools composition editor
    can be undone.
    """

    @classmethod
    def setUpClass(cls):
        cmds.loadPlugin('mayaUsdPlugin')

    def setUp(self):
        # Skip if the Debug Tools component was not shipped in this build; the
        # smoke test above already reports a hard failure when it is expected.
        try:
            import AdskUsdDebug  
        except Exception as e:
            self.skipTest(f"Autodesk USD Debug Tools module not available: {e}")

        cmds.file(force=True, new=True)

        self.stage = Usd.Stage.CreateInMemory()

        # Mirror executeInCmd step 1: track the layer being edited so its inverse
        # edits are recorded by the undo manager.
        mayaUsdLib.UsdUndoManager.trackLayerStates(self.stage.GetRootLayer())

        cmds.select(clear=True)

    def testRemoveOpinionUndoRedo(self):
        '''Removing a single property opinion is undoable and redoable.'''
        from AdskUsdDebug import RemoveOpinion

        prim = self.stage.DefinePrim('/Foo', 'Xform')
        prim.CreateAttribute('radius', Sdf.ValueTypeNames.Double).Set(2.0)
        layerId = self.stage.GetRootLayer().identifier

        self.assertTrue(prim.GetAttribute('radius').HasAuthoredValue())

        nbCmds = cmds.undoInfo(q=True)

        with mayaUsdLib.UsdUndoBlock():
            self.assertTrue(RemoveOpinion(self.stage, layerId, '/Foo.radius'))

        # The opinion is gone and exactly one command was added to the queue.
        self.assertFalse(prim.GetAttribute('radius').HasAuthoredValue())
        self.assertEqual(cmds.undoInfo(q=True), nbCmds + 1)

        # Undo restores the removed opinion (value included).
        cmds.undo()
        self.assertTrue(prim.GetAttribute('radius').HasAuthoredValue())
        self.assertEqual(prim.GetAttribute('radius').Get(), 2.0)

        # Redo removes it again.
        cmds.redo()
        self.assertFalse(prim.GetAttribute('radius').HasAuthoredValue())

    def testRemoveOpinionsUndoRedo(self):
        '''Removing several opinions in one edit is undoable as a single step.'''
        from AdskUsdDebug import OpinionRef, RemoveOpinions

        prim = self.stage.DefinePrim('/Foo', 'Xform')
        prim.CreateAttribute('radius', Sdf.ValueTypeNames.Double).Set(2.0)
        prim.CreateAttribute('count', Sdf.ValueTypeNames.Int).Set(7)
        layerId = self.stage.GetRootLayer().identifier

        nbCmds = cmds.undoInfo(q=True)

        with mayaUsdLib.UsdUndoBlock():
            self.assertTrue(RemoveOpinions(
                self.stage,
                [
                    OpinionRef(layerId, '/Foo.radius'),
                    OpinionRef(layerId, '/Foo.count'),
                ]))

        # Both opinions removed, batched into a single undoable command.
        self.assertFalse(prim.GetAttribute('radius').HasAuthoredValue())
        self.assertFalse(prim.GetAttribute('count').HasAuthoredValue())
        self.assertEqual(cmds.undoInfo(q=True), nbCmds + 1)

        # A single undo restores both opinions.
        cmds.undo()
        self.assertTrue(prim.GetAttribute('radius').HasAuthoredValue())
        self.assertTrue(prim.GetAttribute('count').HasAuthoredValue())
        self.assertEqual(prim.GetAttribute('radius').Get(), 2.0)
        self.assertEqual(prim.GetAttribute('count').Get(), 7)


if __name__ == '__main__':
    unittest.main(verbosity=2)
