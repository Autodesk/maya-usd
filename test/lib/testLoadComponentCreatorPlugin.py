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

import mayaUtils
import fixturesUtils

import os, sys

class LoadComponentCreatorPluginTestCase(unittest.TestCase):
    """
    Verify that the USD component creator plugin can be loaded.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def testPluginLoadable(self):
        # The USD component creator plugin will only be there in certain circumstances so
        # only try and load it if the plugin actually exists. We search in all
        # the plugin paths in env var MAYA_PLUG_IN_PATH.
        if 'MAYA_MODULE_PATH' not in os.environ:
            self.skipTest('MAYA_MODULE_PATH environment variable not set')

        testPlugin = False
        modulePath = os.environ['MAYA_MODULE_PATH']
        for pp in modulePath.split(os.path.pathsep):
            if os.path.exists(pp) and ('usd_component_creator.mod' in os.listdir(pp)):
                testPlugin = True
                break
        
        forceTest = os.environ.get('MAYAUSD_FORCE_CC_TEST', '0') == '1'

        if not testPlugin:
            if forceTest:
                self.fail('USD component creator plugin was not found but MAYAUSD_FORCE_CC_TEST is set. '
                          'MAYA_MODULE_PATH={}'.format(modulePath))
            else:
                self.skipTest('Could not find the USD component creator plugin')

        self.assertTrue(mayaUtils.loadPlugin('mayaUsdPlugin'))
        self.assertTrue(mayaUtils.loadPlugin('usd_component_creator'))
        self.fail('THE TEST WAS ACTUALLY RUN! ' + str(forceTest))


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
