#!/usr/bin/env python

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

from maya import cmds
from maya import mel

import fixturesUtils
import mayaUtils


class MayaUsdPluginUIDeletionTestCase(unittest.TestCase):
    """
    Verify that unloading `mayaUsdPlugin` tears down the UI it created, even
    when removing the preferences tab fails.

    `deleteCustomPrefsTab` is only defined once Maya's preferences window scripts
    have been sourced. In a session that never opens Preferences, an unresolved
    procedure warning can unwind `mayaUsd_pluginUIDeletion` before menu and callback
    teardown runs. 

    This test asserts that the `mayaUsdPlugin` is unloaded in this scenario.
    """

    HOOK = 'addItemsToOutlinerNodePopupMenu'
    CALLBACK = 'mayaUsdMenu_OutlinerPopupCallback'
    RENDER_DESCRIPTION_NODE = 'UsdDefaultRenderDescription'

    @classmethod
    def setUpClass(cls):
        """Prepare a GUI Maya session without loading mayaUsdPlugin yet."""
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False,
                                         loadPlugin=False)

    def _outlinerCallbacks(self):
        """Return callback proc names registered on the outliner popup hook."""
        result = mel.eval('callbacks -listCallbacks -hook "%s"' % self.HOOK)
        return result if result else []

    def _ensurePluginUiLoaded(self):
        """
        Run 'mayaUsd_pluginUICreation' when registerUI did not run after load.

        Batch load (mayaUsd_pluginBatchLoad) leaves menus and callbacks unset;
        CI and maya -c tests need the same UI setup as interactive plugin load.
        """
        if self.CALLBACK in self._outlinerCallbacks():
            return
        mel.eval('source "mayaUsd_pluginUICreation.mel"; mayaUsd_pluginUICreation();')

    def _preparePluginUnload(self):
        """
        Remove UsdDefaultRenderDescription so unload is not blocked.

        Newer Maya versions create this locked node when the plugin loads; it
        must be deleted before force-unload in tests.
        """
        if not cmds.objExists(self.RENDER_DESCRIPTION_NODE):
            return
        cmds.undoInfo(state=False)
        try:
            cmds.lockNode(self.RENDER_DESCRIPTION_NODE, lock=False)
            cmds.delete(self.RENDER_DESCRIPTION_NODE)
        finally:
            cmds.undoInfo(state=True)

    def testUnloadRemovesUIWhenPrefsTabRemovalFails(self):
        """
        Unload must complete and tear down outliner UI if `deleteCustomPrefsTab` fails.

        Replaces `deleteCustomPrefsTab` with a proc that errors, simulating the
        missing-procedure case. Asserts the plugin unloads and the outliner
        callback is removed despite the prefs-tab error.
        """
        self.assertTrue(mayaUtils.loadPlugin('mayaUsdPlugin'))
        self._ensurePluginUiLoaded()

        # mayaUsdMenu_loadui() registers this unconditionally, and
        # mayaUsdMenu_unloadui() -> termOutlinerPopupMenu() removes it.
        self.assertIn(self.CALLBACK, self._outlinerCallbacks())

        # Force the failure instead of relying on Maya's preferences
        # scripts to be unsourced. Replace the existing `deleteCustomPrefsTab`
        # with a new one emitting an error explicitely.
        mel.eval('global proc deleteCustomPrefsTab(string $tab)'
            ' { error "simulated deleteCustomPrefsTab failure"; }')

        # Maya reports the simulated failure, which is quielty catched 
        # so the `mayaUsdPlugin` is unloaded properly.
        self._preparePluginUnload()
        cmds.unloadPlugin('mayaUsdPlugin', force=True)

        self.assertFalse(cmds.pluginInfo('mayaUsdPlugin', query=True, loaded=True))
        self.assertNotIn(self.CALLBACK, self._outlinerCallbacks())


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
