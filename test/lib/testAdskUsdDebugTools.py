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

import ctypes
import os
import sys
import unittest

import maya.cmds as cmds

from pxr import Sdf, Usd

import mayaUsd.lib as mayaUsdLib

import fixturesUtils


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


class testAdskUsdDebugToolsPluginReload(unittest.TestCase):
    """
    Verify that mayaUsdPlugin installs its Debug Tools host on every load and
    removes it on every unload.
    """

    # macOS really unloads the plugin's libraries, and reloading them re-registers their
    # TfTypes, which USD's type registry does not allow and which crashes Maya.
    @unittest.skipIf(sys.platform == 'darwin', 'USD types cannot be re-registered after a library reload')
    def testHostIsUninstalledOnPluginUnload(self):
        '''The plugin installs its ApplicationHost on load and removes it on unload.'''
        # AdskUsdDebugUI is otherwise loaded only through mayaUsdUI, and would unload with it.
        # Load the plugin first so the library's dependencies are already resolved. ctypes
        # never frees the library, so it stays loaded for the rest of the process.
        cmds.loadPlugin('mayaUsdPlugin', quiet=True)
        pluginPath = cmds.pluginInfo('mayaUsdPlugin', query=True, path=True)
        libName = 'AdskUsdDebugUI.dll' if sys.platform == 'win32' else 'libAdskUsdDebugUI.so'
        # The plugin sits a few folders below the MayaUSD module folder, whose lib folder holds the library.
        libPath = None
        folder = os.path.dirname(pluginPath)
        while os.path.dirname(folder) != folder and not libPath:
            candidate = os.path.join(folder, 'lib', libName)
            libPath = candidate if os.path.isfile(candidate) else None
            folder = os.path.dirname(folder)
        if not libPath:
            self.skipTest(f"{libName} not found above {pluginPath}")

        # Adsk::UsdDebug::ApplicationHost::instance()
        symbol = ('?instance@ApplicationHost@UsdDebug@Adsk@@SAAEAV123@XZ' if sys.platform == 'win32'
                  else '_ZN4Adsk8UsdDebug15ApplicationHost8instanceEv')
        instance = getattr(ctypes.CDLL(libPath), symbol)
        instance.restype = ctypes.c_void_p
        def hostVtable():
            # The first word of the host object is its vtable pointer.
            return ctypes.c_void_p.from_address(instance()).value

        self.addCleanup(cmds.loadPlugin, 'mayaUsdPlugin', quiet=True)
        for _ in range(2):
            cmds.loadPlugin('mayaUsdPlugin', quiet=True)
            self.assertTrue(cmds.pluginInfo('mayaUsdPlugin', query=True, loaded=True))
            loadedVtable = hostVtable()
            # The plugin cannot unload while the undo queue holds its data types.
            fixturesUtils.deleteUsdSettingsSingletons()
            cmds.unloadPlugin('mayaUsdPlugin', force=True)
            self.assertFalse(cmds.pluginInfo('mayaUsdPlugin', query=True, loaded=True))

            # Once the plugin's host is removed, instance() falls back to the default host.
            self.assertNotEqual(loadedVtable, hostVtable(), "The plugin did not install or remove its host.")


if __name__ == '__main__':
    unittest.main(verbosity=2)
