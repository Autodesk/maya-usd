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
    Verify that unloading mayaUsdPlugin tears down the UI it created, even
    when removing the preferences tab fails.

    mayaUsd_pluginUIDeletion() used to call deleteCustomPrefsTab() before
    mayaUsdMenu_unloadui(). deleteCustomPrefsTab is only defined once Maya's
    preferences window scripts have been sourced, so in a session that never
    opens the Preferences window it raised "Cannot find procedure", MEL
    unwound the proc, and the menu and callback teardown never ran. The
    leftover callbacks and menu items then outlived the unloaded plugin
    library, which crashed Maya later on when it walked its widget tree.

    This must run in GUI Maya: Maya only invokes the procedures registered
    through MFnPlugin::registerUI in interactive mode, and calls
    mayaUsd_pluginBatchLoad/Unload in batch mode instead.
    """

    HOOK = 'addItemsToOutlinerNodePopupMenu'
    CALLBACK = 'mayaUsdMenu_OutlinerPopupCallback'

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False,
                                         loadPlugin=False)

    def _outlinerCallbacks(self):
        result = mel.eval('callbacks -listCallbacks -hook "%s"' % self.HOOK)
        return result if result else []

    def testUnloadRemovesUIWhenPrefsTabRemovalFails(self):
        self.assertTrue(mayaUtils.loadPlugin('mayaUsdPlugin'))

        # mayaUsdMenu_loadui() registers this unconditionally, and
        # mayaUsdMenu_unloadui() -> termOutlinerPopupMenu() removes it.
        self.assertIn(self.CALLBACK, self._outlinerCallbacks())

        # Force the failure rather than relying on Maya's preferences scripts
        # happening to be unsourced. A MEL error unwinds the caller exactly
        # like an unresolved procedure does. Nothing restores the original
        # procedure, so this must remain the only test in this module.
        mel.eval('global proc deleteCustomPrefsTab(string $tab)'
                 ' { error "simulated deleteCustomPrefsTab failure"; }')

        # Maya reports the simulated failure, but it must not prevent the
        # plugin from unloading nor the UI teardown from running. The
        # assertions below cover both.
        try:
            cmds.unloadPlugin('mayaUsdPlugin', force=True)
        except RuntimeError:
            pass

        self.assertFalse(cmds.pluginInfo('mayaUsdPlugin', query=True, loaded=True))
        self.assertNotIn(self.CALLBACK, self._outlinerCallbacks())


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
