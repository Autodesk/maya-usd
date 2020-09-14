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

from ufeTestUtils import mayaUtils
import ufe

import unittest

@unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2022', 'ChildFilterTestCase is only available in Maya with UFE preview version 0.2.22 and greater')
class ChildFilterTestCase(unittest.TestCase):
    '''Verify the ChildFilter USD implementation.
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

        # Open ballset.ma scene in test-samples
        mayaUtils.openGroupBallsScene()

        # Clear selection to start off
        cmds.select(clear=True)

    def testFilteredChildren(self):
        # Check that we have six balls
        propsPath = ufe.PathString.path('|transform1|proxyShape1,/Ball_set/Props')
        propsItem = ufe.Hierarchy.createItem(propsPath)
        propsHier = ufe.Hierarchy.hierarchy(propsItem)
        self.assertEqual(6, len(propsHier.children()))

        # Deactivate Ball_3 (which will remove it from children)
        ball3Path = ufe.PathString.path('|transform1|proxyShape1,/Ball_set/Props/Ball_3')
        ball3Hier = ufe.Hierarchy.createItem(ball3Path)
        cmds.delete('|transform1|proxyShape1,/Ball_set/Props/Ball_3')

        # Props should now have 5 children and ball3 should not be one of them.
        children = propsHier.children()
        self.assertEqual(5, len(children))
        self.assertNotIn(ball3Hier, children)

        # Ensure we have one USD child filter
        rid = ufe.RunTimeMgr.instance().getId('USD')
        usdHierHndlr = ufe.RunTimeMgr.instance().hierarchyHandler(rid)
        cf = usdHierHndlr.childFilter()
        self.assertEqual(1, len(cf))

        # Make sure the USD hierarchy handler has an inactive prims filter
        self.assertEqual('InactivePrims', cf[0].name)

        # Toggle "Inactive Prims" on and get the filtered children
        # (with inactive prims) and verify ball3 is one of them.
        cf[0].value = True
        children = propsHier.filteredChildren(cf)
        self.assertEqual(6, len(children))
        self.assertIn(ball3Hier, children)
