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

import maya.cmds as cmds
import maya.mel as mel

from mayaUsd.lib import UsdDefaultRenderSettings

class VerifyUsdRenderSetupTestCase(unittest.TestCase):
    """ Test the Usd Render Setup. """

    @classmethod
    def setUpClass(cls):
        loaded = cmds.loadPlugin('mayaUsdPlugin', quiet=True)
        if loaded != ['mayaUsdPlugin']:
            raise RuntimeError('mayaUsd plugin load failed.')

    def testWindowCommandExists(self):
        # Verify that the render setup window command exists, which indicates that the render setup UI is available.
        self.assertTrue(mel.eval('exists mayaUsdRenderSetupWindow'))

    def testPythonBindings(self):
        # Verify that we can load the python bindings and call at least one method.
        try:
            import AdskUsdRenderSetup as rs
        except ImportError as e:
            self.fail(f"Failed to import my_module: {e}")

        self.assertEqual(rs.__name__, 'AdskUsdRenderSetup')
        self.assertTrue(rs.__version_info__ >= (0, 0, 1))

        stage = UsdDefaultRenderSettings.getUsdStage()
        self.assertIsNotNone(rs.GetAllRenderSettingsPaths(stage))

        prim = rs.GetActiveRenderSettings(stage)
        self.assertTrue(prim)