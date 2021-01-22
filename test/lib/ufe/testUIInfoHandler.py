#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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

import os

import maya.cmds as cmds

import mayaUtils
import ufe

import unittest

class UIInfoHandlerTestCase(unittest.TestCase):
    '''Verify the UIInfoHandler USD implementation.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    def setUp(self):
        ''' Called initially to set up the Maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # Open ballset.ma scene in testSamples
        mayaUtils.openGroupBallsScene()

        # Clear selection to start off
        cmds.select(clear=True)

    def testUIInfo(self):
        rid = ufe.RunTimeMgr.instance().getId('USD')
        ufeUIInfo = ufe.UIInfoHandler.uiInfoHandler(rid)
        self.assertIsNotNone(ufeUIInfo)

        # We won't test the actual string. Just make sure its not empty.
        self.assertNotEqual(0, len(ufeUIInfo.getLongRunTimeLabel()))

        # Deactivate one of the balls.
        cmds.delete('|transform1|proxyShape1,/Ball_set/Props/Ball_3')

        # Now ask for the treeViewCellInfo and test that some values were set
        # (because the ball is inactive).
        ball3Path = ufe.PathString.path('|transform1|proxyShape1,/Ball_set/Props/Ball_3')
        ball3Hier = ufe.Hierarchy.createItem(ball3Path)

        # A return value of true means something was set.
        # Note: operator= in Python creates a new variable that shares the reference
        #       of the original object. So don't create initTextFgClr from ci.textFgColor.
        ci = ufe.CellInfo()
        initTextFgClr = ufe.Color3f(ci.textFgColor.r(), ci.textFgColor.g(), ci.textFgColor.b())
        changed = ufeUIInfo.treeViewCellInfo(ball3Hier, ci)
        self.assertTrue(changed)

        # Strikeout should be be set and the text fg color changed.
        self.assertTrue(ci.fontStrikeout)
        self.assertTrue(initTextFgClr != ci.textFgColor)

        # Ball_3 should have a specific node type icon set and no badge.
        icon = ufeUIInfo.treeViewIcon(ball3Hier)
        self.assertEqual(icon.baseIcon, "out_USD_Sphere.png")
        self.assertFalse(icon.badgeIcon)    # empty string
