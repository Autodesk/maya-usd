#!/usr/bin/env python

#
# Copyright 2025 Autodesk
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
import ufeUtils

from maya import cmds
from maya import standalone

import ufe
from ufe.extensions import lookdevXUfe # Not using "as" in this module.
import mayaUsd.ufe

import os
import unittest


class CapabilityHandlerTestCase(unittest.TestCase):
    '''Verify the CapabilityHandler USD implementation.
    '''

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
        ''' Called initially to set up the Maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # Open ballset.ma scene in testSamples
        mayaUtils.openGroupBallsScene()

        # Clear selection to start off
        cmds.select(clear=True)

    def testCapabilities(self):
        rid = ufe.RunTimeMgr.instance().getId('USD')
        ldxCaps = ufe.extensions.lookdevXUfe.CapabilityHandler.get(rid)
        self.assertIsNotNone(ldxCaps)

        self.assertTrue(ldxCaps.hasCapability(ufe.extensions.lookdevXUfe.CapabilityHandler.kCanPromoteToMaterial))
        self.assertTrue(ldxCaps.hasCapability(ufe.extensions.lookdevXUfe.CapabilityHandler.kCanPromoteInputAtTopLevel))
        self.assertTrue(ldxCaps.hasCapability(ufe.extensions.lookdevXUfe.CapabilityHandler.kCanHaveNestedNodeGraphs))
        self.assertFalse(ldxCaps.hasCapability(ufe.extensions.lookdevXUfe.CapabilityHandler.kCanUseCreateShaderCommandForComponentConnections))

if __name__ == '__main__':
    unittest.main(verbosity=2)
