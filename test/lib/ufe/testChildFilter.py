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

import fixturesUtils
import mayaUtils

from maya import cmds
from maya import standalone

import mayaUsd.ufe

import ufe

import unittest


class ChildFilterTestCase(unittest.TestCase):
    '''Verify the ChildFilter USD implementation.
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

    def testFilteredChildren(self):
        # Check that we have six balls
        propsPath = ufe.PathString.path('|transform1|proxyShape1,/Ball_set/Props')
        propsItem = ufe.Hierarchy.createItem(propsPath)
        propsHier = ufe.Hierarchy.hierarchy(propsItem)
        self.assertEqual(6, len(propsHier.children()))

        # Deactivate Ball_3 (which will remove it from children)
        ball3Path = ufe.PathString.path('|transform1|proxyShape1,/Ball_set/Props/Ball_3')
        ball3Hier = ufe.Hierarchy.createItem(ball3Path)
        ballSetPrim = mayaUsd.ufe.ufePathToPrim('|transform1|proxyShape1,/Ball_set/Props/Ball_3')
        ballSetPrim.SetActive(False)

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

    def testProxyShapeFilteredChildren(self):
        # Check that the proxy shape has a single child, Ball_set.
        psPath = ufe.PathString.path('|transform1|proxyShape1')
        psItem = ufe.Hierarchy.createItem(psPath)
        psHier = ufe.Hierarchy.hierarchy(psItem)
        psChildren = psHier.children()

        self.assertEqual(len(psChildren), 1)
        ballSetPathStr = '|transform1|proxyShape1,/Ball_set'
        ballSetPath = ufe.PathString.path(ballSetPathStr)
        ballSetItem = ufe.Hierarchy.createItem(ballSetPath)
        self.assertEqual(ballSetItem, psChildren[0])

        # Get the USD child filter.  We know from testFilteredChildren() that
        # it has a single filter flag, 'InactivePrims'.
        rid = ufe.RunTimeMgr.instance().getId('USD')
        usdHierHndlr = ufe.RunTimeMgr.instance().hierarchyHandler(rid)
        cf = usdHierHndlr.childFilter()

        # The filtered children are the same as default children.
        psFilteredChildren = psHier.filteredChildren(cf)
        self.assertEqual(len(psFilteredChildren), 1)
        self.assertEqual(ballSetItem, psFilteredChildren[0])

        # Inactivate Ball_set.  Default children will be empty, filtered
        # children will remain the same.
        ballSetPrim = mayaUsd.ufe.ufePathToPrim(ballSetPathStr)
        ballSetPrim.SetActive(False)

        psChildren = psHier.children()
        self.assertEqual(len(psChildren), 0)

        psFilteredChildren = psHier.filteredChildren(cf)
        self.assertEqual(len(psFilteredChildren), 1)

        # Now get the Maya child filter.  Because the proxy shape hierarchy
        # handler overrides the Maya child filter and adds the 'InactivePrims'
        # child filter flag, it should be on the Maya child filter.
        rid = ufe.RunTimeMgr.instance().getId('Maya-DG')
        mayaHierHndlr = ufe.RunTimeMgr.instance().hierarchyHandler(rid)
        cf = mayaHierHndlr.childFilter()
        self.assertEqual(1, len(cf))
        self.assertEqual('InactivePrims', cf[0].name)

        psFilteredChildren = psHier.filteredChildren(cf)
        self.assertEqual(len(psFilteredChildren), 1)
        self.assertEqual(ballSetItem, psFilteredChildren[0])

if __name__ == '__main__':
    unittest.main(verbosity=2)
