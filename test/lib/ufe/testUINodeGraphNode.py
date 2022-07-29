#!/usr/bin/env python

#
# Copyright 2022 Autodesk
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

from maya import cmds
from maya import standalone

import ufe
import mayaUsd.ufe

import os
import unittest


class UINodeGraphNodeTestCase(unittest.TestCase):
    '''Verify the UINodeGraphNode USD implementation.
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

    def testUIInfo(self):
        ball3Path = ufe.PathString.path('|transform1|proxyShape1,/Ball_set/Props/Ball_3')
        ball3SceneItem = ufe.Hierarchy.createItem(ball3Path)

        uiNodeGraphNode = ufe.UINodeGraphNode.uiNodeGraphNode(ball3SceneItem)

        self.assertFalse(uiNodeGraphNode.hasPosition())
        pos0 = uiNodeGraphNode.getPosition()
        self.assertEqual(pos0.x(), 0)
        self.assertEqual(pos0.y(), 0)
        pos1 = ufe.Vector2f(10, 20)
        uiNodeGraphNode.setPosition(pos1)
        self.assertTrue(uiNodeGraphNode.hasPosition())
        pos2 = uiNodeGraphNode.getPosition()
        self.assertEqual(pos1.x(), pos2.x())
        self.assertEqual(pos1.y(), pos2.y())
        uiNodeGraphNode.setPosition(13, 41)
        self.assertTrue(uiNodeGraphNode.hasPosition())
        pos3 = uiNodeGraphNode.getPosition()
        self.assertEqual(pos3.x(), 13)
        self.assertEqual(pos3.y(), 41)
        pos4 = ufe.Vector2f(21, 20)
        cmd = uiNodeGraphNode.setPositionCmd(pos4)
        cmd.execute()
        self.assertTrue(uiNodeGraphNode.hasPosition())
        pos5 = uiNodeGraphNode.getPosition()
        self.assertEqual(pos4.x(), pos5.x())
        self.assertEqual(pos4.y(), pos5.y())

if __name__ == '__main__':
    unittest.main(verbosity=2)
