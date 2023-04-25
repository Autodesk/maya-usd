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
import testUtils

from maya import cmds
from maya import standalone

import ufe

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

        self.messageHandler = mayaUtils.TestProxyShapeUpdateHandler('|transform1|proxyShape1')

        # Clear selection to start off
        cmds.select(clear=True)

    def tearDown(self):
        self.messageHandler.terminate()

    # Helper to avoid copy-pasting the entire test
    def doPosAndSizeTests(self, hasFunc, setFunc, getFunc, cmdFunc):
        self.messageHandler.snapshot()
                                          
        # Test hasFunc and getFunc
        self.assertFalse(hasFunc())
        pos0 = getFunc()
        self.assertEqual(pos0.x(), 0)
        self.assertEqual(pos0.y(), 0)

        # Test setFunc with vector
        pos1 = ufe.Vector2f(10, 20)
        setFunc(pos1)
        self.assertTrue(hasFunc())
        pos2 = getFunc()
        self.assertEqual(pos1.x(), pos2.x())
        self.assertEqual(pos1.y(), pos2.y())

        # Test setFunc with scalars
        setFunc(13, 41)
        self.assertTrue(hasFunc())
        pos3 = getFunc()
        self.assertEqual(pos3.x(), 13)
        self.assertEqual(pos3.y(), 41)

        # Test cmdFunc
        pos4 = ufe.Vector2f(21, 20)
        cmd = cmdFunc(pos4)
        cmd.execute()
        self.assertTrue(hasFunc())
        pos5 = getFunc()
        self.assertEqual(pos4.x(), pos5.x())
        self.assertEqual(pos4.y(), pos5.y())

        # None of these changes should force a render refresh:
        self.assertTrue(self.messageHandler.isUnchanged())

    def testPosition(self):
        ball3Path = ufe.PathString.path('|transform1|proxyShape1,/Ball_set/Props/Ball_3')
        ball3SceneItem = ufe.Hierarchy.createItem(ball3Path)

        uiNodeGraphNode = ufe.UINodeGraphNode.uiNodeGraphNode(ball3SceneItem)
        self.doPosAndSizeTests(uiNodeGraphNode.hasPosition, uiNodeGraphNode.setPosition,
            uiNodeGraphNode.getPosition, uiNodeGraphNode.setPositionCmd)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4100',
                     'Size interface only available in Ufe preview version greater equal to 4.0.100, or 0.5.0.')
    def testSize(self):
        ball3Path = ufe.PathString.path('|transform1|proxyShape1,/Ball_set/Props/Ball_3')
        ball3SceneItem = ufe.Hierarchy.createItem(ball3Path)

        if(hasattr(ufe, "UINodeGraphNode_v4_1")):
            uiNodeGraphNode = ufe.UINodeGraphNode_v4_1.uiNodeGraphNode(ball3SceneItem)
        else:
            uiNodeGraphNode = ufe.UINodeGraphNode.uiNodeGraphNode(ball3SceneItem)

        self.doPosAndSizeTests(uiNodeGraphNode.hasSize, uiNodeGraphNode.setSize,
            uiNodeGraphNode.getSize, uiNodeGraphNode.setSizeCmd)

if __name__ == '__main__':
    unittest.main(verbosity=2)
