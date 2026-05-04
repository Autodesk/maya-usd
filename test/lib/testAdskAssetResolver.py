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
from pxr import Ar as pxrAr
import AdskAssetResolver as ar
from maya import cmds
import fixturesUtils
import mayaUtils

class AdskAssetResolverTestCase(unittest.TestCase):

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)
        # Ensure the idle queue is running so that MGlobal::executeTaskOnIdle
        # callbacks fire when cmds.flushIdleQueue() is called (needed on Linux).
        cmds.flushIdleQueue(resume=True)
        # Ensure the queue is clear before starting the test so that undo stack is predictable,
        # some idle jobs can append commands on the stack.
        cmds.flushIdleQueue()
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
                        
    def setUp(self):
        cmds.file(new=True, force=True)

    def testAssetResolver(self):
        adskResolver = pxrAr.GetUnderlyingResolver()
        self.assertIsNotNone(adskResolver, "No underlying asset resolver was found")       
        try:
            # This is Autodesk Asset Resolver api. If it doesn't work, AR is not properly loaded.
            ar.VersionInfo() 
        except Exception as e:
            self.fail(f"Autodesk Asset Resolver API not available or failed to load. {e}")

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
    