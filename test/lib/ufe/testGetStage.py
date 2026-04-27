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

import fixturesUtils
import mayaUtils
import mayaUsd
import mayaUsd_createStageWithNewLayer

from maya import standalone

import ufe

import unittest


class GetStageTestCase(unittest.TestCase):
    '''Verify GetStage interface.'''

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
        ''' Called initially to set up the maya test environment '''
        self.assertTrue(self.pluginsLoaded)

    def testGetStageEmptyPath(self):
        '''Test getStage with an empty path.'''
        stage = mayaUsd.ufe.getStage("")
        self.assertIsNone(stage)

    def testGetStage(self):
        '''Test getStage for a valid stage path gives the same answer as GetPrim().GetStage().'''
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        primStage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        self.assertIsNotNone(primStage)
        
        stage = mayaUsd.ufe.getStage(psPathStr)
        self.assertIsNotNone(stage)
        self.assertEqual(stage, primStage)


if __name__ == '__main__':
    unittest.main(verbosity=2)
