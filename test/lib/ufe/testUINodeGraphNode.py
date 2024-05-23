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
        self.assertEqual(pos0, ufe.Vector2f())

        # Test setFunc with vector
        pos1 = ufe.Vector2f(10, 20)
        setFunc(pos1)
        self.assertTrue(hasFunc())
        pos2 = getFunc()
        self.assertEqual(pos1, pos2)

        # Test setFunc with scalars
        setFunc(13, 41)
        self.assertTrue(hasFunc())
        pos3 = getFunc()
        self.assertEqual(pos3, ufe.Vector2f(13, 41))

        # Test cmdFunc
        pos4 = ufe.Vector2f(21, 20)
        cmd = cmdFunc(pos4)
        cmd.execute()
        self.assertTrue(hasFunc())
        pos5 = getFunc()
        self.assertEqual(pos4, pos5)

        # None of these changes should force a render refresh:
        self.assertTrue(self.messageHandler.isUnchanged())

    def testPosition(self):
        ball3Path = ufe.PathString.path('|transform1|proxyShape1,/Ball_set/Props/Ball_3')
        ball3SceneItem = ufe.Hierarchy.createItem(ball3Path)

        uiNodeGraphNode = ufe.UINodeGraphNode.uiNodeGraphNode(ball3SceneItem)
        self.doPosAndSizeTests(uiNodeGraphNode.hasPosition, uiNodeGraphNode.setPosition,
            uiNodeGraphNode.getPosition, uiNodeGraphNode.setPositionCmd)

    @unittest.skipIf(os.getenv('UFE_UINODEGRAPHNODE_HAS_SIZE', 'NOT-FOUND') not in ('1', 'TRUE'),
                     'Testing node graph size needs newer has/get/size size methods.')
    def testSize(self):
        ball3Path = ufe.PathString.path('|transform1|proxyShape1,/Ball_set/Props/Ball_3')
        ball3SceneItem = ufe.Hierarchy.createItem(ball3Path)

        if(hasattr(ufe, "UINodeGraphNode_v4_1")):
            uiNodeGraphNode = ufe.UINodeGraphNode_v4_1.uiNodeGraphNode(ball3SceneItem)
        else:
            uiNodeGraphNode = ufe.UINodeGraphNode.uiNodeGraphNode(ball3SceneItem)

        self.doPosAndSizeTests(uiNodeGraphNode.hasSize, uiNodeGraphNode.setSize,
            uiNodeGraphNode.getSize, uiNodeGraphNode.setSizeCmd)

    @unittest.skipIf(os.getenv('UFE_UINODEGRAPHNODE_HAS_DISPLAYCOLOR', 'NOT-FOUND') not in ('1', 'TRUE'),
                     'Testing node graph display color needs newer has/get/set display color methods.')
    def testDisplayColor(self):
        ball5Path = ufe.PathString.path('|transform1|proxyShape1,/Ball_set/Props/Ball_5')
        ball5SceneItem = ufe.Hierarchy.createItem(ball5Path)
        uiNodeGraphNode = ufe.UINodeGraphNode.uiNodeGraphNode(ball5SceneItem)

        self.messageHandler.snapshot()

        # Test has and get methods.
        self.assertFalse(uiNodeGraphNode.hasDisplayColor())
        color0 = uiNodeGraphNode.getDisplayColor()
        self.assertEqual(color0, ufe.Color3f())

        # Test set method with Color3f
        clr1 = ufe.Color3f(1,0,0)
        uiNodeGraphNode.setDisplayColor(clr1)
        self.assertTrue(uiNodeGraphNode.hasDisplayColor())
        clr2 = uiNodeGraphNode.getDisplayColor()
        self.assertEqual(clr1, clr2)
 
        # Test set method with 3 floats.
        uiNodeGraphNode.setDisplayColor(0, 1, 0)
        self.assertTrue(uiNodeGraphNode.hasDisplayColor())
        clr3 = uiNodeGraphNode.getDisplayColor()
        self.assertEqual(clr3, ufe.Color3f(0, 1, 0))

        # Test command method.
        clr4 = ufe.Color3f(0, 0, 1)
        cmd = uiNodeGraphNode.setDisplayColorCmd(clr4)
        self.assertIsNotNone(cmd)
        cmd.execute()
        self.assertTrue(uiNodeGraphNode.hasDisplayColor())
        clr5 = uiNodeGraphNode.getDisplayColor()
        self.assertEqual(clr4, clr5)

        # None of these changes should force a render refresh:
        self.assertTrue(self.messageHandler.isUnchanged())

if __name__ == '__main__':
    unittest.main(verbosity=2)
