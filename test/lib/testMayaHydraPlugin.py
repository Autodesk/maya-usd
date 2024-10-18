#!/usr/bin/env python

#
# Copyright 2024 Autodesk
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

import mayaUtils
import fixturesUtils

import os, sys

class MayaHydraPluginCheckTestCase(unittest.TestCase):
    """
    Verify that the MayaHydra plugin can be loaded.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2024, 'MayaHydra requires Maya 2024 or greater.')
    def testPluginLoadable(self):
        # The MayaHydra plugin will only be there in certain circumstances so
        # only try and load it if the plugin actually exists. We search in all
        # the plugin paths in env var MAYA_PLUG_IN_PATH.
        testMayaHydra = False
        if 'MAYA_PLUG_IN_PATH' in os.environ:
            pluginSuffix = ''
            if sys.platform == 'win32':
                pluginSuffix = '.mll'
            elif sys.platform == 'darwin':
                pluginSuffix = '.bundle'
            elif sys.platform.startswith('linux'):
                pluginSuffix = '.so'
            pluginPath = os.environ['MAYA_PLUG_IN_PATH']
            for pp in pluginPath.split(os.path.pathsep):
                if os.path.exists(pp) and ('mayaHydra{suf}'.format(suf=pluginSuffix) in os.listdir(pp)):
                    testMayaHydra = True
                    break

        if not testMayaHydra:
            print('testMayaHydraPlugin: skipping test as mayaHydra plugin not found.')
            self.skipTest('Could not find mayaHydra plugin')

        self.assertTrue(mayaUtils.loadPlugin('mayaHydra'))
        self.assertTrue(mayaUtils.loadPlugin('mayaHydraSceneBrowser'))


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
