#!/usr/bin/env python

#
# Copyright 2021 Autodesk
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
import testUtils
import ufeUtils
import usdUtils

import mayaUsd.ufe

from pxr import Kind
from pxr import Usd

from maya import cmds
from maya import standalone

import ufe

import os
import unittest

class UngroupCmdTestCase(unittest.TestCase):

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

    def testUngroupUndoRedo(self):
        '''Verify multiple undo/redo.'''
        pass

    def testUngroupMultipleGroupItems(self):
        '''Verify ungrouping of multiple group nodes.'''
        pass

    def testUngroupAbsolute(self):
        '''Verify -absolute flag.'''
        pass

    def testUngroupRelative(self):
        '''Verify -relative flag.'''
        pass

    def testUngroupWorld(self):
        '''Verify -world flag.'''
        pass

    def testUngroupParent(self):
        '''Verify -parent flag.'''
        pass

    def testUngroupProxyShape(self):
        '''Verify ungrouping of the proxyShape.'''
        pass

    def testUngroupLeaf(self):
        '''Verify ungrouping of a leaf node.'''
        pass

if __name__ == '__main__':
    unittest.main(verbosity=2)
